// ---------------------------------------------------------------------------
//                       main.cpp  —  micro-ROS robot core loop
// ---------------------------------------------------------------------------
//  Responsibility split:
//    • config.hpp     — pins / geometry / gains
//    • electronics.*  — common base class
//    • ultrasonic.*   — HC-SR04 distance (METRES)
//    • motor.*        — Cytron MDD10A duty-cycle driver
//    • encoder.*      — quadrature wheel encoder (m/s, sign-corrected)
//    • pid.*          — discrete PID with anti-windup
//    • main.cpp       — micro-ROS plumbing and the 100 Hz control loop (CORE 0)
//    • imu.*          — BNO085 orientation / gyro / linear-accel (CORE 1)
//
//  Units convention:  ALL velocities are m/s, ALL distances are metres.
//  No [-1, 1] velocity normalisation anywhere.  The only [-1, 1] saturation
//  in the codebase is on PWM duty cycle, which is a hardware physical limit.
// ---------------------------------------------------------------------------
//
//  Changes vs previous version:
//    1. Slew-rate limited setpoint  — kills 0->V_MAX step transients
//    2. Velocity feedforward         — PID only corrects residual error
//    3. Ultrasonic divider           — protects 100 Hz PID timing
//    4. fabsf() near-zero compare    — replaces fragile float == 0 test
//    5. BNO085 IMU added — quaternion, gyro, linear accel on /sensors/imu
//    6. IMU moved to CORE 1          — sh2_service() can no longer stall
//                                      the 100 Hz control loop on I2C.
//    7. Loop pacing moved to END     — timestamp at start, busy-wait at end.
// ---------------------------------------------------------------------------

#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "pico/mutex.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "pico_uart_transports.h"
}

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/int32.h>
#include <geometry_msgs/msg/twist.h>
#include <sensor_msgs/msg/range.h>
#include <sensor_msgs/msg/imu.h>
#include <rosidl_runtime_c/string_functions.h>

#include "config.hpp"
#include "ultrasonic.hpp"
#include "motor.hpp"
#include "encoder.hpp"
#include "pid.hpp"
#include "imu.hpp"

#define RCCHECK(fn) { rcl_ret_t rc = (fn); \
    if (rc != RCL_RET_OK) { printf("RCL error %ld at %s:%d\n", rc, __FILE__, __LINE__); } }

// ---------------------------------------------------------------------------
//                              micro-ROS handles
// ---------------------------------------------------------------------------
rcl_subscription_t cmd_vel_sub;
rcl_publisher_t    usrm_front_pub, usrm_back_pub, usrm_left_pub, usrm_right_pub;
rcl_publisher_t    enc_left_pub,   enc_right_pub;
rcl_publisher_t    imu_pub;

geometry_msgs__msg__Twist cmd_vel_msg;
sensor_msgs__msg__Range   usrm_front_msg, usrm_back_msg, usrm_left_msg, usrm_right_msg;
std_msgs__msg__Int32      enc_left_msg, enc_right_msg;
sensor_msgs__msg__Imu     imu_msg;

rclc_executor_t executor;
rclc_support_t  support;
rcl_allocator_t allocator;
rcl_node_t      node;

// ---------------------------------------------------------------------------
//                              Shared state — cmd_vel
// ---------------------------------------------------------------------------
volatile float    target_vel_l_mps     = 0.0f;
volatile float    target_vel_r_mps     = 0.0f;
volatile uint64_t last_cmd_vel_time_us = 0;

// ---------------------------------------------------------------------------
//                          Shared state — IMU (Core 1 → Core 0)
// ---------------------------------------------------------------------------
//  Core 1 runs sh2_service() in a tight loop and copies the IMU readings
//  into g_imu_snap under g_imu_mutex.  Core 0 grabs an atomic snapshot of
//  the struct under the same mutex at IMU publish time.
//
//  The mutex is held for ~50 cycles (one struct copy), so cross-core
//  contention is negligible even at 2 kHz polling on core 1.
// ---------------------------------------------------------------------------
struct ImuSnapshot {
    float qi, qj, qk, qr;     // rotation vector (quaternion)
    float gx, gy, gz;         // calibrated gyroscope (rad/s)
    float ax, ay, az;         // linear acceleration (m/s², gravity removed)
};
static ImuSnapshot g_imu_snap = {0.0f, 0.0f, 0.0f, 1.0f,   // identity quat
                                 0.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f};
static mutex_t        g_imu_mutex;
static volatile bool  g_imu_ready = false;   // set by core 1 once SH2 is up

//  IMU instance lives at file scope so core 1 can reach it.  Constructor
//  only assigns members (no HW access), so it is safe to default-construct
//  here; init() runs on core 1.
static IMU IMU_SENSOR("IMU", "ON", i2c0, IMU_SDA, IMU_SCL);

// ---------------------------------------------------------------------------
//                              cmd_vel callback
// ---------------------------------------------------------------------------
void cmd_vel_callback(const void* msg_in)
{
    const auto* msg = (const geometry_msgs__msg__Twist*)msg_in;

    float v_mps = (float)msg->linear.x;     // in m/s
    float w_rps = (float)msg->angular.z;    // in rad/s

    target_vel_l_mps     = v_mps - w_rps * (WHEEL_BASE_M * 0.5f);
    target_vel_r_mps     = v_mps + w_rps * (WHEEL_BASE_M * 0.5f);
    last_cmd_vel_time_us = time_us_64();
}

// ---------------------------------------------------------------------------
//                              Helpers
// ---------------------------------------------------------------------------
static void init_range_msg(sensor_msgs__msg__Range* msg, uint8_t rad_type,
                        float fov, float min_r, float max_r)
{
    msg->radiation_type = rad_type;
    msg->field_of_view  = fov;
    msg->min_range      = min_r;
    msg->max_range      = max_r;
    msg->range          = 0.0f;
}

static void set_msg_stamp(std_msgs__msg__Header* header)
{
    uint64_t now_us = time_us_64();
    header->stamp.sec     = now_us / 1000000ULL;
    header->stamp.nanosec = (now_us % 1000000ULL) * 1000ULL;
}

//  Slew-rate limit a single setpoint towards its target.  Caps how fast the
//  commanded velocity can change per loop iteration, which prevents step
//  inputs from teleop / brain node from causing PID transients.
static inline float slew(float current, float target, float max_step)
{
    float delta = target - current;
    if (delta >  max_step) delta =  max_step;
    if (delta < -max_step) delta = -max_step;
    return current + delta;
}

//  Saturate a value to a closed interval.  Used for clamping duty cycle
//  after feedforward + PID summation.
static inline float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

// ---------------------------------------------------------------------------
//                            Core 1 entry point
// ---------------------------------------------------------------------------
//  Core 1 owns the IMU exclusively:
//    1. Initialise I2C + SH2 transport (also runs sh2_open on this core).
//    2. Spin pumping sh2_service() and copying decoded values into the
//       shared snapshot under g_imu_mutex.
//
//  Polling at ~2 kHz (sleep 500 µs) is far faster than the 100 Hz IMU
//  report rate, so we never miss a packet, and the core is idle the rest
//  of the time so power draw stays low.
//
//  If init() fails, core 1 enters a quiet idle loop — core 0 will see
//  g_imu_ready == false and simply skip IMU publishing.
// ---------------------------------------------------------------------------
static void core1_entry()
{
    if (!IMU_SENSOR.init()) {
        printf("[core1] IMU init failed — IMU publish will be skipped\n");
        while (true) {
            tight_loop_contents();
        }
    }
    IMU_SENSOR.ShowStatus();
    g_imu_ready = true;

    while (true) {
        IMU_SENSOR.update();    // pumps sh2_service() — may block briefly on I2C

        // Copy decoded values into the shared snapshot.  Mutex is held only
        // long enough to copy 10 floats — well under a microsecond.
        mutex_enter_blocking(&g_imu_mutex);
        g_imu_snap.qi = IMU_SENSOR.getQuatI();
        g_imu_snap.qj = IMU_SENSOR.getQuatJ();
        g_imu_snap.qk = IMU_SENSOR.getQuatK();
        g_imu_snap.qr = IMU_SENSOR.getQuatReal();
        g_imu_snap.gx = IMU_SENSOR.getGyroX();
        g_imu_snap.gy = IMU_SENSOR.getGyroY();
        g_imu_snap.gz = IMU_SENSOR.getGyroZ();
        g_imu_snap.ax = IMU_SENSOR.getLinAccelX();
        g_imu_snap.ay = IMU_SENSOR.getLinAccelY();
        g_imu_snap.az = IMU_SENSOR.getLinAccelZ();
        mutex_exit(&g_imu_mutex);

        sleep_us(500);          // ~2 kHz service rate, plenty for 100 Hz IMU
    }
}

// ---------------------------------------------------------------------------
//                                   main
// ---------------------------------------------------------------------------
int main()
{
    stdio_init_all();
    sleep_ms(2000);

    // ---- Hardware (core 0 owned) ----
    Ultrasonic USRM_FRONT("Front", "ON", USRM_FRONT_TRIG, USRM_FRONT_ECHO);
    Ultrasonic USRM_BACK ("Back",  "ON", USRM_BACK_TRIG,  USRM_BACK_ECHO );
    Ultrasonic USRM_RIGHT("Right", "ON", USRM_RIGHT_TRIG, USRM_RIGHT_ECHO);
    Ultrasonic USRM_LEFT ("Left",  "ON", USRM_LEFT_TRIG,  USRM_LEFT_ECHO );

    Motor      MOTOR("MOTORS", "ON", L_DIR, L_PWM, R_DIR, R_PWM,
                    /*invert_left*/  true,
                    /*invert_right*/ true,
                    /*trim_left*/    LEFT_MOTOR_TRIM,
                    /*trim_right*/   RIGHT_MOTOR_TRIM);

    Encoder    LEFT_ENCODER ("L_ENC", "ON",
                            ENC_L_A, ENC_L_B,
                            GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                            /*invert*/ true);
    Encoder    RIGHT_ENCODER("R_ENC", "ON",
                            ENC_R_A, ENC_R_B,
                            GEAR_REDUCTION, WHEEL_DIAMETER_M, ENCODER_PPR,
                            /*invert*/ true);

    // PID per wheel — m/s setpoint vs m/s measured -> PID *correction* term
    // (NOT raw duty cycle anymore).  Output limited to ±0.5 so the
    // feedforward stays in charge and PID can only nudge.
    PID LEFT_PID (PID_KP, PID_KI, PID_KD, -0.5f, 0.5f, 0.5f);
    PID RIGHT_PID(PID_KP, PID_KI, PID_KD, -0.5f, 0.5f, 0.5f);

    USRM_FRONT.ShowStatus(); USRM_BACK.ShowStatus();
    USRM_RIGHT.ShowStatus(); USRM_LEFT.ShowStatus();
    MOTOR.ShowStatus();
    LEFT_ENCODER.ShowStatus(); RIGHT_ENCODER.ShowStatus();

    // ---- Launch core 1 (IMU loop) ----------------------------------------
    // Done BEFORE micro-ROS init so the IMU is already producing data by
    // the time the first control tick runs.  The handshake is purely the
    // g_imu_ready flag — we don't block waiting for it.
    mutex_init(&g_imu_mutex);
    multicore_launch_core1(core1_entry);

    // ---- micro-ROS init (core 0) ----
    rmw_uros_set_custom_transport(
        true, NULL,
        pico_serial_transport_open,  pico_serial_transport_close,
        pico_serial_transport_write, pico_serial_transport_read);

    allocator = rcl_get_default_allocator();
    rclc_support_init(&support, 0, NULL, &allocator);
    rclc_node_init_default(&node, "pico", "", &support);

    rclc_subscription_init_default(
        &cmd_vel_sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "/cmd_vel");

    rclc_publisher_init_default(&usrm_front_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
        "/sensors/ultrasonic/usrm_front");
    rclc_publisher_init_default(&usrm_back_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
        "/sensors/ultrasonic/usrm_back");
    rclc_publisher_init_default(&usrm_left_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
        "/sensors/ultrasonic/usrm_left");
    rclc_publisher_init_default(&usrm_right_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
        "/sensors/ultrasonic/usrm_right");
    rclc_publisher_init_default(&enc_left_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "/sensors/encoders/left_ticks");
    rclc_publisher_init_default(&enc_right_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "/sensors/encoders/right_ticks");
    rclc_publisher_init_default(&imu_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu),
        "/sensors/imu");

    init_range_msg(&usrm_front_msg, 0, 0.26f, 0.02f, 4.00f);
    init_range_msg(&usrm_back_msg,  0, 0.26f, 0.02f, 4.00f);
    init_range_msg(&usrm_left_msg,  0, 0.26f, 0.02f, 4.00f);
    init_range_msg(&usrm_right_msg, 0, 0.26f, 0.02f, 4.00f);

    rosidl_runtime_c__String__assign(&usrm_front_msg.header.frame_id, "ultrasonic_front_link");
    rosidl_runtime_c__String__assign(&usrm_back_msg.header.frame_id,  "ultrasonic_back_link");
    rosidl_runtime_c__String__assign(&usrm_left_msg.header.frame_id,  "ultrasonic_left_link");
    rosidl_runtime_c__String__assign(&usrm_right_msg.header.frame_id, "ultrasonic_right_link");
    rosidl_runtime_c__String__assign(&imu_msg.header.frame_id, "imu_link");

    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(
        &executor, &cmd_vel_sub, &cmd_vel_msg, &cmd_vel_callback, ON_NEW_DATA);

    last_cmd_vel_time_us = time_us_64();
    printf("micro-ROS ready, listening on /cmd_vel\n");

    // -----------------------------------------------------------------------
    //                              Main loop @ 100 Hz   (CORE 0)
    // -----------------------------------------------------------------------
    uint8_t  usrm_index = 0;
    uint8_t  usrm_div_counter = 0;
    uint8_t  imu_div_counter = 0;

    // Smoothed (slew-rate-limited) setpoints.  These are what actually feed
    // the PID — NOT the raw target_vel_*_mps values written by cmd_vel.
    float setpoint_l_smooth = 0.0f;
    float setpoint_r_smooth = 0.0f;

    constexpr float    LOOP_DT_S        = 0.01f;                       // 100 Hz
    constexpr float    MAX_VEL_STEP_MPS = MAX_ACCEL_MPS2 * LOOP_DT_S;  // 0.005 m/s
    constexpr uint64_t LOOP_PERIOD_US   = 10000ULL;                    // 10 ms

    while (true)
    {
        // ===== 0. Mark the start of this control tick =====
        uint64_t loop_start = time_us_64();

        // ===== 1. Pump the ROS executor =====
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(2));

        // ===== 2a. Fire one ultrasonic per Nth tick (round-robin) =====
        //  Spaced out so a blocking HC-SR04 echo doesn't corrupt PID timing
        //  every loop.
        if (usrm_div_counter == 0) {
            switch (usrm_index) {
                case 0:
                    usrm_front_msg.range = USRM_FRONT.update();
                    set_msg_stamp(&usrm_front_msg.header);
                    RCCHECK(rcl_publish(&usrm_front_pub, &usrm_front_msg, NULL));
                    break;
                case 1:
                    usrm_back_msg.range = USRM_BACK.update();
                    set_msg_stamp(&usrm_back_msg.header);
                    RCCHECK(rcl_publish(&usrm_back_pub, &usrm_back_msg, NULL));
                    break;
                case 2:
                    usrm_right_msg.range = USRM_RIGHT.update();
                    set_msg_stamp(&usrm_right_msg.header);
                    RCCHECK(rcl_publish(&usrm_right_pub, &usrm_right_msg, NULL));
                    break;
                case 3:
                    usrm_left_msg.range = USRM_LEFT.update();
                    set_msg_stamp(&usrm_left_msg.header);
                    RCCHECK(rcl_publish(&usrm_left_pub, &usrm_left_msg, NULL));
                    break;
            }
            usrm_index = (usrm_index + 1) % 4;
        }
        usrm_div_counter = (usrm_div_counter + 1) % USRM_DIVIDER;

        // ===== 2b. IMU runs on core 1 — just snapshot and publish here =====
        if (imu_div_counter == 0 && g_imu_ready) {
            ImuSnapshot snap;
            mutex_enter_blocking(&g_imu_mutex);
            snap = g_imu_snap;                  // tiny struct copy under lock
            mutex_exit(&g_imu_mutex);

            imu_msg.orientation.x         = snap.qi;
            imu_msg.orientation.y         = snap.qj;
            imu_msg.orientation.z         = snap.qk;
            imu_msg.orientation.w         = snap.qr;
            imu_msg.angular_velocity.x    = snap.gx;
            imu_msg.angular_velocity.y    = snap.gy;
            imu_msg.angular_velocity.z    = snap.gz;
            imu_msg.linear_acceleration.x = snap.ax;
            imu_msg.linear_acceleration.y = snap.ay;
            imu_msg.linear_acceleration.z = snap.az;
            set_msg_stamp(&imu_msg.header);
            RCCHECK(rcl_publish(&imu_pub, &imu_msg, NULL));
        }
        imu_div_counter = (imu_div_counter + 1) % IMU_DIVIDER;

        // ===== 3. Snapshot raw setpoint (m/s) + cmd_vel watchdog =====
        float setpoint_l_raw = target_vel_l_mps;
        float setpoint_r_raw = target_vel_r_mps;
        if (loop_start - last_cmd_vel_time_us > CMD_VEL_TIMEOUT_US) {
            setpoint_l_raw = 0.0f;
            setpoint_r_raw = 0.0f;
        }

        // Saturate raw setpoint to physical max BEFORE slew-rate limiting,
        // so the smoother can't be asked to reach an unreachable target.
        setpoint_l_raw = clampf(setpoint_l_raw, -V_MAX_MPS, V_MAX_MPS);
        setpoint_r_raw = clampf(setpoint_r_raw, -V_MAX_MPS, V_MAX_MPS);

        // ===== 4. Directional safety E-stop =====
        float v_avg         = 0.5f * (setpoint_l_raw + setpoint_r_raw);
        bool  want_forward  = (v_avg >  DIRECTION_DEADZONE_MPS);
        bool  want_backward = (v_avg < -DIRECTION_DEADZONE_MPS);
        float d_front       = usrm_front_msg.range;
        float d_back        = usrm_back_msg.range;
        if (want_forward  && d_front > 0.0f && d_front < FRONT_STOP_DIST_M) {
            setpoint_l_raw = 0.0f; setpoint_r_raw = 0.0f;
        }
        if (want_backward && d_back  > 0.0f && d_back  < BACK_STOP_DIST_M) {
            setpoint_l_raw = 0.0f; setpoint_r_raw = 0.0f;
        }

        // ===== 5. Slew-rate limit setpoint =====
        setpoint_l_smooth = slew(setpoint_l_smooth, setpoint_l_raw, MAX_VEL_STEP_MPS);
        setpoint_r_smooth = slew(setpoint_r_smooth, setpoint_r_raw, MAX_VEL_STEP_MPS);

        // ===== 6. Closed-loop velocity control =====
        //
        //   setpoint (m/s) ---+--> [Feedforward: setpoint / V_MAX] -----+
        //                     |                                        |
        //                     +--> [PID error vs measured] -- correction
        //                                                              |
        //                                      +-----------------------+
        //                                      v
        //                               duty cycle ∈ [-1, 1]
        //
        float meas_l = LEFT_ENCODER .get_vel();
        float meas_r = RIGHT_ENCODER.get_vel();

        float duty_l, duty_r;

        constexpr float STOP_THRESHOLD_MPS = 1e-3f;
        bool commanded_stop = (fabsf(setpoint_l_smooth) < STOP_THRESHOLD_MPS) &&
                              (fabsf(setpoint_r_smooth) < STOP_THRESHOLD_MPS);

        if (commanded_stop) {
            LEFT_PID .reset();
            RIGHT_PID.reset();
            duty_l = 0.0f;
            duty_r = 0.0f;
        } else {
            float ff_l = setpoint_l_smooth / V_MAX_MPS;
            float ff_r = setpoint_r_smooth / V_MAX_MPS;

            float pid_l = LEFT_PID .calculate(setpoint_l_smooth, meas_l);
            float pid_r = RIGHT_PID.calculate(setpoint_r_smooth, meas_r);

            duty_l = clampf(ff_l + pid_l, -1.0f, 1.0f);
            duty_r = clampf(ff_r + pid_r, -1.0f, 1.0f);
        }

        // ===== 7. Drive motors =====
        MOTOR.move(duty_l, duty_r);

        // ===== 8. Publish encoder ticks =====
        enc_left_msg .data = LEFT_ENCODER .get_count();
        enc_right_msg.data = RIGHT_ENCODER.get_count();
        RCCHECK(rcl_publish(&enc_left_pub,  &enc_left_msg,  NULL));
        RCCHECK(rcl_publish(&enc_right_pub, &enc_right_msg, NULL));

        // ===== 9. Pace at 100 Hz =====
        //  Busy-wait until 10 ms has elapsed since loop_start.  If the work
        //  above ever overruns 10 ms (e.g. an exceptionally slow ultrasonic),
        //  we skip the wait and run straight into the next tick — there is
        //  no time compensation, by design.  Watch for this with a logic
        //  analyser on GP_x if you suspect overruns.
        while ((time_us_64() - loop_start) < LOOP_PERIOD_US) {
            tight_loop_contents();
        }
    }
    return 0;
}
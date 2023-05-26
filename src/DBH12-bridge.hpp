#include <Arduino.h>
#define HANDLER_DELAY 50
#define maxPWMValue (int) (255.0* 0.98)
#define speedFromPercentage(percent)                      \
    {                                                     \
        uint((float)percent  * (float)maxPWMValue) \
    }
enum class DBH12Bridge_Direction
{
    CCW,
    CW
};

enum class DBH12Bridge_handler_operation_type
{
    ReachSpeed,
    Braking,
    Idle,
};
class pendingOperation
{
public:
    uint32_t start;
    uint32_t stop;
    float accelerationRamp;
    DBH12Bridge_handler_operation_type type;
};

class DBH12Bridge
{
   
    static occurenceCount = 0;
    int _pin_in1;
    int _pin_in2;
    int _pin_en;
    int _pin_ct;

   
    int pwmChannelCW ;
    int pwmChannelCCW ;
    const int resolution = 8;
    pendingOperation _pendingOperation;

    uint _currentSpeed;
    uint _speedSetpoint;
    float _current;
    DBH12Bridge_Direction _direction;
    float _maximumCurrentAllowed;

    uint32_t _startBraking;

    void _initGPIO()
    {
        
        //printf("function %s\n", __FUNCTION__);
        printf("set pin %d to %s\n", this->_pin_in1,"output");
        pinMode(this->_pin_in1, OUTPUT);
        printf("set pin %d to %s\n", this->_pin_in2,"output");
        pinMode(this->_pin_in2, OUTPUT);
        printf("set pin %d to %s\n", this->_pin_en,"output");
        pinMode(this->_pin_en, OUTPUT);
        printf("set pin %d to %s\n", this->_pin_ct,"input");
        pinMode(this->_pin_ct, INPUT);
        //analogWriteResolution(8);

        ledcSetup(pwmChannelCW, pwmFrequency, resolution);
        ledcSetup(pwmChannelCCW, pwmFrequency, resolution);
        ledcAttachPin(this->_pin_in1, pwmChannelCW);
        ledcAttachPin(this->_pin_in2, pwmChannelCCW);
    }
    void _getCurrent()
    {
        //printf("function %s\n", __FUNCTION__);
        // CT Output Voltage(V)=Current(A) x 0.155
        int millivolts = analogReadMilliVolts(this->_pin_ct);
        this->_current = (float)millivolts / 0.155 * this->currentSensorDivider;
    }
    void _driveCurrentSpeed()
    {
        //printf("function %s\n", __FUNCTION__);
        if (this->_direction == DBH12Bridge_Direction::CW)
        {
            //analogWrite(this->_pin_in1, this->_currentSpeed);
            ledcWrite(pwmChannelCW, this->_currentSpeed);
            ledcWrite(pwmChannelCCW, 0);
            printf("driveSpeed %d dir:%s\n",this->_currentSpeed, "CW");
        }
        if (this->_direction == DBH12Bridge_Direction::CCW)
        {
            //analogWrite(this->_pin_in2, this->_currentSpeed);
            ledcWrite(pwmChannelCCW, this->_currentSpeed);
            ledcWrite(pwmChannelCW, 0);
            printf("driveSpeed %d dir:%s\n",this->_currentSpeed, "CCW");
        }
    }

public:
    /**
     * @brief normaly the formula is CT Output Voltage(V)=Current(A) x 0.155 but the adc only accept voltage between 0 and 3.3v
     * so you should put a divider bridge to rectify the the voltage. the formula change to Current(A)= Voltage(V) / 0.155 * divider
     *
     */
    float currentSensorDivider;
    int pwmFrequency = 500;
    DBH12Bridge(int pin_in1, int pin_in2, int pin_en, int pin_ct) : _pin_in1(pin_in1), _pin_in2(pin_in2), _pin_en(pin_en), _pin_ct(pin_ct)
    {
        this->_currentSpeed = 0;
        this->_speedSetpoint = 0;
        this->_direction = DBH12Bridge_Direction::CW;
        this->currentSensorDivider = 1;
        this->pwmChannelCW=(DBH12Bridge::occurenceCount*2);
        this->pwmChannelCCW=(DBH12Bridge::occurenceCount*2)+1;
        DBH12Bridge::occurenceCount++;
    }
    void init()
    {
        //printf("function %s\n", __FUNCTION__);
        this->_initGPIO();
    }
    void handler()
    {
        static uint32_t lastHandlerTime = 0;
        uint32_t elapsedTime = millis() - lastHandlerTime;
        //printf("function %s currentSpeed=%d speedSetpoint=%d\n", __FUNCTION__,this->_currentSpeed,this->_speedSetpoint);
        this->_getCurrent();
        if ( this->_maximumCurrentAllowed > 0) {
            if (this->_current > this->_maximumCurrentAllowed)
            {
                
                this->stop();
                this->_pendingOperation.type = DBH12Bridge_handler_operation_type::Idle;
            }
        }
        switch (this->_pendingOperation.type)
        {
        case DBH12Bridge_handler_operation_type::Braking:
        {
            uint32_t maxStep = speedFromPercentage(this->_pendingOperation.accelerationRamp * ((float)(elapsedTime) / 1000.0));
            //printf("function %s maxStep=%d\n", __FUNCTION__, maxStep);
            maxStep = std::min(maxStep, this->_currentSpeed);
            //printf("function %s maxStep=%d\n", __FUNCTION__, maxStep);
            this->_currentSpeed -= maxStep;
            if (this->_currentSpeed == 0)
            {
                this->_pendingOperation.stop=millis();
                this->_pendingOperation.type = DBH12Bridge_handler_operation_type::Idle;
                printf(" operation Braking during:%d\n",this->_pendingOperation.stop-this->_pendingOperation.start);
                this->stop();
            }
            break;
        }
        case DBH12Bridge_handler_operation_type::ReachSpeed:
        {
            uint32_t maxStep = speedFromPercentage(this->_pendingOperation.accelerationRamp * ((float)(elapsedTime) / 1000.0));
            //printf("function %s maxStep=%d\n", __FUNCTION__, maxStep);
            maxStep = std::min((int)maxStep, std::abs((int)this->_speedSetpoint - (int)this->_currentSpeed));
            //printf("function %s maxStep=%d\n", __FUNCTION__, maxStep);
            if (this->_speedSetpoint > this->_currentSpeed)
            {
                this->_currentSpeed += maxStep;
            }
            else if (this->_speedSetpoint < this->_currentSpeed)
            {
                this->_currentSpeed -= maxStep;
            }
            else
            {
                this->_pendingOperation.stop=millis();
                this->_pendingOperation.type = DBH12Bridge_handler_operation_type::Idle;
                printf(" operation ReachSpeed during:%d\n",this->_pendingOperation.stop-this->_pendingOperation.start);
            }
            break;
        }

        default:
            break;
        }
        lastHandlerTime = millis();

        this->_driveCurrentSpeed();
    }

    /**
     * @brief brake the motor
     *
     */
    void brake()
    {
        //printf("function %s\n", __FUNCTION__);
        this->_startBraking = millis();
        digitalWrite(this->_pin_in1, LOW);
        digitalWrite(this->_pin_in1, LOW);
    }
    /**
     * @brief brake smoothly
     *
     * @param accelerationRamp during one second how much speed can be decreased
     */
    void asyncSmoothBrake(float accelerationRamp)
    {
        //printf("function %s\n", __FUNCTION__);
        this->handler();
        this->_startBraking = millis();
        this->_speedSetpoint = 0;
        this->_pendingOperation.start=millis();
        this->_pendingOperation.type = DBH12Bridge_handler_operation_type::Braking;
        this->_pendingOperation.accelerationRamp = accelerationRamp;
    }
    void smoothBrake(float accelerationRamp)
    {
        //printf("function %s\n", __FUNCTION__);
        this->asyncSmoothBrake(accelerationRamp);
        while (this->_pendingOperation.type != DBH12Bridge_handler_operation_type::Idle)
        {
            this->handler();
            delay(HANDLER_DELAY);
        }
    }
    /**
     * @brief  stop control of motor
     *
     */
    void stop()
    {
        //printf("function %s\n", __FUNCTION__);
        digitalWrite(this->_pin_in1, LOW);
        digitalWrite(this->_pin_in2, LOW);
        digitalWrite(this->_pin_en, LOW);
    }

    /**
     * @brief Set the Speed of the motor without acceleration
     *
     * @param speed is a percentage so it should be between 0 and 1
     */
    void setSpeed(float speed, DBH12Bridge_Direction direction)
    {
        //printf("function %s\n", __FUNCTION__);
        speed = std::max(speed, (float) 0);
        speed = std::min(speed, (float) 1);
        this->_direction = direction;
        ;
        this->_currentSpeed = speedFromPercentage(speed);
        this->_driveCurrentSpeed();
    }

    /**
     * @brief Set the Smooth Speed of motor the motor will accelerate to reach the specified speed
     *
     * @param speed is a percentage so it should be between 0 and 1
     * @param accelerationRamp a(x) value represent afine function of acceleration ex a=1 mean reach 100% in 1s a=0.5 mean reach 100% in 2s
     */
    void asyncSetSmoothSpeed(float speed, float accelerationRamp, DBH12Bridge_Direction direction)
    {
        //printf("function %s\n", __FUNCTION__);
        this->handler();
        this->_speedSetpoint = speedFromPercentage(speed);
        this->_direction = direction;
          this->_pendingOperation.start=millis();
        this->_pendingOperation.type = DBH12Bridge_handler_operation_type::ReachSpeed;
        this->_pendingOperation.accelerationRamp = accelerationRamp;
    }
    void SetSmoothSpeed(float speed, float accelerationRamp, DBH12Bridge_Direction direction)
    {
        //printf("function %s\n", __FUNCTION__);
        this->asyncSetSmoothSpeed(speed, accelerationRamp, direction);
        while (this->_pendingOperation.type != DBH12Bridge_handler_operation_type::Idle)
        {
            this->handler();
            delay(HANDLER_DELAY);
        }
    }
    /**
     * @brief Set the Speed of motor dive By feeling back current. A Pid will drive the motor to respect the specified current value
     *
     * @param current
     */
    void asyncSetSpeedByCurrent(float current, DBH12Bridge_Direction direction)
    {
        //printf("function %s\n", __FUNCTION__);
         this->handler();
    }
    void setSpeedByCurrent(float current, DBH12Bridge_Direction direction)
    {
        //printf("function %s\n", __FUNCTION__);
        this->asyncSetSpeedByCurrent(current, direction);
        while (this->_pendingOperation.type != DBH12Bridge_handler_operation_type::Idle)
        {
            this->handler();
            delay(HANDLER_DELAY);
        }
    }
};
#define EARTH_GRAVITY_MS2	9.80665
#define SCALE_MULTIPLIER	0.004
#define DATA_FORMAT		0x31
#define BW_RATE			0x2C
#define POWER_CTL		0X2D
#define BW_RATE_1600HZ		0X0F
#define BW_RATE_800HZ		0X0E
#define BW_RATE_400HZ		0X0D
#define BW_RATE_200HZ		0X0C
#define BW_RATE_100HZ		0X0B
#define BW_RATE_50HZ		0X0A
#define BW_RATE_25HZ		0X09
#define RANGE_2G		0X00
#define RANGE_4G		0X01
#define RANGE_8G		0X02
#define RANGE_16G		0X03
#define MEASURE			0X08
#define AXES_DATA		0X32
#define ACCEL_ADDR		0x53

struct coord {
   double x;
   double y;
   double z;
} coord; 

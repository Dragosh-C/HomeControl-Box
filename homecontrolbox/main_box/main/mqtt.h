#define BOX_ID "1111"

void mqtt_start(void);
void mqtt_publish(const char *topic, const char *message);
void relays_func(void);
void set_relays(int8_t relay, int8_t state);
void set_dimmer(int duty);


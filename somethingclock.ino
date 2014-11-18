const int hours_out = 8;
const int minutes_out = 9;
const int seconds_out = 10;
const int temp_out = 11;
const int pressure_out = 12;

const size_t NUM_HEADER_BYTES = 7;
const char BEGIN[] = "\x08\x06\x07\x05\x03\x00\x09";

const int LED_PINS[] = {50, 51, 52, 53};

void seconds_meter(int seconds)
{
  // Add a bit more to get low numbers into the range that
  // even registers
  analogWrite(seconds_out, ((seconds+5.0)/(59+5))*255.0);    
}

void minutes_meter(int minutes)
{
  analogWrite(minutes_out, ((minutes+5.0)/(59+5))*255.0);  
}

void hours_meter(int hours)
{
  analogWrite(hours_out, ((hours+1.0)/(11+1))*255.0);  
}

void temp_meter(int temp)
{
    // temp range: [0,100]
    analogWrite(temp_out, (int)((temp/100.0)*255.0)); 
}

void baro_meter(float pressure)
{
  // range: [29.0, 31.0]
  // 29.0 --> 0
  // 31.0 --> 255 
  analogWrite(pressure_out, ((pressure - 29.0)/(31.0 - 29.0))*255.0);  
}

struct CityData
{
  char hour;
  char minute;
  char second;
  
  float temperature;
  float pressure;
};

class Application {
  CityData* cities;
  size_t num_cities;
  
  size_t num_header_bytes_in_a_row;
  
  size_t active_city;
  
public:
  
  void init(CityData* cities, size_t num_cities){
    this->cities = cities;
    this->num_cities = num_cities;
    this->active_city = 0;
    
    for (size_t i=0; i<(sizeof(CityData)*num_cities); ++i){
      ((char *)(this->cities))[i] = 0;
      this->cities[i].hour = i+2;
      this->cities[i].minute = i*10;      
    }

    this->num_header_bytes_in_a_row=0;
  }
  
  void process_serial(){
    while (Serial.available() > 0) {
      if (this->num_header_bytes_in_a_row < NUM_HEADER_BYTES) {
        char new_byte = Serial.read();
        if (new_byte == BEGIN[num_header_bytes_in_a_row]){
          this->num_header_bytes_in_a_row++;
        } else {
          Serial.println("Dropping...");
          Serial.flush();
          this->num_header_bytes_in_a_row = 0;
        }
      } else { // We're ready to read a whole thing 
        break;
      }
    }
    
    if (this->num_header_bytes_in_a_row == NUM_HEADER_BYTES
        &&
        Serial.available() >= (sizeof(CityData)+1)){
      size_t index = Serial.read();
      if (index >= this->num_cities) {
        return;
      }
      
      Serial.readBytes((char *)(this->cities + index), sizeof(CityData));
      Serial.println("Processed input.");
      Serial.print((int)this->cities[index].second);
      Serial.println();
      this->num_header_bytes_in_a_row = 0;
    }
  }
  
  void update_board(){
    CityData c = this->cities[this->active_city];
    
    hours_meter(c.hour % 12);
    minutes_meter(c.minute); 
    seconds_meter(c.second);
  
    temp_meter(c.temperature);
    baro_meter(c.pressure);  
  }
  
  void set_active_city(size_t i){
    this->active_city = i;
    
    for (size_t j=0; j<sizeof(LED_PINS); ++j){
      digitalWrite(LED_PINS[j], (i==j ? HIGH : LOW));
    }
  }
};

Application app;
CityData cities[4];

void city_0(){app.set_active_city(0);}
void city_1(){app.set_active_city(1);}
void city_2(){app.set_active_city(2);}
void city_3(){app.set_active_city(3);}

void setup_isr()
{
  for (size_t i; i<sizeof(LED_PINS); i++){
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
    
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
  pinMode(20, INPUT_PULLUP);
  
  attachInterrupt(0, city_0, FALLING);
  attachInterrupt(1, city_1, FALLING);
  attachInterrupt(2, city_2, FALLING);
  attachInterrupt(3, city_3, FALLING);
}

void setup()
{
  Serial.begin(1200);

  app.init(cities, 4);
    
  pinMode(hours_out, OUTPUT);
  pinMode(minutes_out, OUTPUT);
  pinMode(seconds_out, OUTPUT);

  setup_isr();
  
  Serial.println("Initialized.");
}

void loop()
{
  app.process_serial();
  app.update_board();
}

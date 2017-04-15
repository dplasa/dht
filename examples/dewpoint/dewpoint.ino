//
// This example compares a precise and a sloppy method
// of calculating the dew point
//
void setup()
{
  Serial.begin(9600);
  Serial.println(F("DHTxx DEW POINT TEST PROGRAM"));

  float maxdelta = 0.00;

  for (int t = 70; t >= 0; --t)
  {
    for (int h = 0; h < 100; ++h)
    {
      float temperature = t * 10;
      float humidity = h * 10;
      
      /// SLOPPY
      const float a = 17.271;
      float b = 2377; //237.7;
      float tempF = temperature;
      float tmp = (a * tempF) / (b + tempF) + log(humidity * 0.001);
      b *= 0.1f;
      float Tsloppy = (b * tmp) / (a - tmp);

      // PRECISE
      float RATIO = 373.15 / (273.15 + temperature * 0.1f);
      float RHS = -7.90298 * (RATIO - 1);
      RHS += 5.02808 * log10(RATIO);
      RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / RATIO ))) - 1) ;
      RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
      RHS += 3.0057149; //log10(1013.246);

      // factor -3 is to adjust units - Vapor Pressure SVP * humidity
      float VP = pow(10, RHS - 4) * humidity;

      // (2) DEWPOINT = F(Vapor Pressure)
      //float T = log(VP/0.61078);   // temp var
      float T = log(VP) + 0.49301845;   // temp var
      float Tprecise = (241.88 * T) / (17.558 - T);

      if (fabs(Tprecise - Tsloppy)  > maxdelta)
      {
        maxdelta = fabs(Tprecise - Tsloppy);
        Serial.print(F("Dew Point deviation: ")); 
        Serial.print(int (maxdelta * 1000)); 
        Serial.print(F("mK at: T=")) ; 
        Serial.print(t); Serial.print(F("°C / RH=")); 
        Serial.print(h); 
        Serial.println(F("%"));
      }
    }
  }
}

void loop()
{
}
//
// END OF FILE
//

#include <mbed.h>

DigitalInOut col_1(PE_9);
DigitalInOut col_2(PE_10);
DigitalInOut col_3(PE_11);

DigitalIn row_1(PE_12);
DigitalIn row_2(PE_13);
DigitalIn row_3(PE_14);
DigitalIn row_4(PE_15);

DigitalIn zone_1(PE_4);
DigitalIn zone_2(PE_5);
DigitalIn zone_3(PE_6);
DigitalIn zone_4(PE_7);

DigitalOut led_green(PD_12);
DigitalOut led_orange(PD_13);
DigitalOut led_red(PD_14);
DigitalOut led_blue(PD_15);

DigitalOut led_power(PD_0);

DigitalOut relay(PE_8);

Serial wi_fi(PA_2, PA_3);

DigitalInOut cols[] = {col_1, col_2, col_3};
DigitalIn rows[] = {row_1, row_2, row_3, row_4};

static void serial_setup(void)
{
  wi_fi.baud(115200);
}

static void gpio_setup(void)
{
  col_1.input();
  col_2.input();
  col_3.input();

  col_1.mode(PullNone);
  col_2.mode(PullNone);
  col_3.mode(PullNone);

  row_1.mode(PullUp);
  row_2.mode(PullUp);
  row_3.mode(PullUp);
  row_4.mode(PullUp);

  zone_1.mode(PullUp);
  zone_2.mode(PullUp);
  zone_3.mode(PullUp);
  zone_4.mode(PullUp);
}

int GetKeyPressed()
{
  int row, col;

  for (col = 0; col < 3; col++)
  {
    cols[col].output();
    cols[col].mode(PullDown);

    for (row = 0; row < 4; row++)
    {
      int state = rows[row].read();

      if (!state)
      {
        cols[col].input();
        cols[col].mode(PullNone);

        return row * 3 + col;
      }
    }

    cols[col].input();
    cols[col].mode(PullNone);
  }

  return 0xFF;
}

uint16_t GetCodeFromInput(char inputFromKeypad[])
{
  return (((1000 * inputFromKeypad[0]) + (100 * inputFromKeypad[1]) + (10 * inputFromKeypad[2]) + inputFromKeypad[3]));
}

int main()
{
  serial_setup();
  gpio_setup();

  bool isSecurityEnabled = false;

  uint16_t securityCode = 487;

  char inputFromKeypad[] = "####";
  uint8_t inputCarriage = 0;
  uint8_t previousKey = 0XFF;
  uint8_t pressedKeyCode;

  led_power = 1;

  while (1)
  {
    if (inputCarriage != 4)
    {
      pressedKeyCode = GetKeyPressed();

      wait(0.3);

      if (pressedKeyCode != previousKey)
      {
        if (pressedKeyCode == 0xFF || pressedKeyCode == 11)
        {
          previousKey = 0xFF;
        }
        else
        {
          inputFromKeypad[inputCarriage] = pressedKeyCode;
          previousKey = pressedKeyCode;
          inputCarriage++;
        }
      }
    }

    if (inputCarriage == 4)
    {
      if (GetKeyPressed() == 11 && GetCodeFromInput(inputFromKeypad) == securityCode)
      {
        isSecurityEnabled = !isSecurityEnabled;
        strcpy(inputFromKeypad, "####");
        inputCarriage = 0;
        relay = isSecurityEnabled ? 1 : 0;
      }
      else if (GetKeyPressed() == 11 && GetCodeFromInput(inputFromKeypad) != securityCode)
      {
        strcpy(inputFromKeypad, "####");
        inputCarriage = 0;
      }
    }

    if (isSecurityEnabled) {
      led_orange = zone_1 ? 0 : 1;
      led_green = zone_2 ? 0 : 1;
      led_red = zone_3 ? 0 : 1;
      led_blue = zone_4 ? 0 : 1;

      if (zone_1 || zone_2 || zone_3 || zone_4) {
        wi_fi.puts("AZ|" + zone_1 + zone_2 + zone_3 + zone_4);
      }
    } else {
      led_orange = 0;
      led_green = 0;
      led_red = 0;
      led_blue = 0;
    }
  }
}
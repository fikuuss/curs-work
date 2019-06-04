#include <mbed.h>

// конфигурация портов подключённых к столбцам клавиатуры
DigitalInOut col_1(PE_9);
DigitalInOut col_2(PE_10);
DigitalInOut col_3(PE_11);

// конфигурация портов подключённых к строкам клавиатуры
DigitalIn row_1(PE_12);
DigitalIn row_2(PE_13);
DigitalIn row_3(PE_14);
DigitalIn row_4(PE_15);

// конфигурация портов для опроса подключаемых датчиков
DigitalIn zone_1(PE_4);
DigitalIn zone_2(PE_5);
DigitalIn zone_3(PE_6);
DigitalIn zone_4(PE_7);

// конфигурация портов для светодиодной индикации
DigitalOut led_green(PD_12);
DigitalOut led_orange(PD_13);
DigitalOut led_red(PD_14);
DigitalOut led_blue(PD_15);

DigitalOut led_power(PD_0);

// конфигурация порта для управления реле
DigitalOut relay(PE_8);

// конфигурация порта для управления пьезодинамиком
DigitalOut buzer(PE_2);

// конфигурация последовательного порта USART2 платы
Serial wi_fi(PA_2, PA_3);

DigitalInOut cols[] = {col_1, col_2, col_3};
DigitalIn rows[] = {row_1, row_2, row_3, row_4};

static void serial_setup(void)
{
  // установка бодрэйта последовательного порта
  wi_fi.baud(115200);
}

static void gpio_setup(void)
{
  // установка режима работы портов подключённых к столбцам клавиатуры
  col_1.input();
  col_2.input();
  col_3.input();

  // установка "подтяжки" портов подключённых к столбцам клавиатуры
  col_1.mode(PullNone);
  col_2.mode(PullNone);
  col_3.mode(PullNone);

  // установка "подтяжки" портов подключённых к строкам клавиатуры
  row_1.mode(PullUp);
  row_2.mode(PullUp);
  row_3.mode(PullUp);
  row_4.mode(PullUp);

  // установка "подтяжки" портов для опроса подключаемых датчиков
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
    // устанавливаем логический ноль ноль на col столбце матричной клавиатуры
    cols[col].output();
    cols[col].mode(PullDown);

    for (row = 0; row < 4; row++)
    {
      // считываем состояние цифрового входа row столбца
      int state = rows[row].read();

      if (!state)
      {
        // сбрасываем состояние col столбца
        cols[col].input();
        cols[col].mode(PullNone);

        // возвращаем значение индекса нажатой кнопки
        return row * 3 + col;
      }
    }

    // сбрасываем состояние col столбца
    cols[col].input();
    cols[col].mode(PullNone);
  }

  return 0xFF;
}

uint16_t GetCodeFromInput(char inputFromKeypad[])
{
  // преобразование массива символов в число
  return (((1000 * inputFromKeypad[0]) + (100 * inputFromKeypad[1]) + (10 * inputFromKeypad[2]) + inputFromKeypad[3]));
}

int main()
{
  // вызываем функции конфигурации последовального и цифровых портов
  serial_setup();
  gpio_setup();

  // создаём вспомогательные перменные
  bool isSecurityEnabled = false;

  uint16_t securityCode = 487;

  char inputFromKeypad[] = "####";
  uint8_t inputCarriage = 0;
  uint8_t previousKey = 0XFF;
  uint8_t pressedKeyCode;

  // "включаем" светодиод сигнализирующий об успешном запуске системы
  led_power = 1;

  while (1)
  {
    // проверяем нажатие клавиши на клавиатуре пока не будет введено четыре цифры
    if (inputCarriage != 4)
    {
      pressedKeyCode = GetKeyPressed();

      // задержка для нивелирования "дребезга" контактов кнопки
      wait(0.3);

      // проверяем, что отжали нажатую кнопку
      if (pressedKeyCode != previousKey)
      {
        if (pressedKeyCode == 0xFF || pressedKeyCode == 11)
        {
          previousKey = 0xFF;
        }
        else
        {
          // записываем значение полученной кнопки в массив символов и сдвигаем каретку ввода на единицу
          inputFromKeypad[inputCarriage] = pressedKeyCode;
          previousKey = pressedKeyCode;
          inputCarriage++;
        }
      }
    }

    if (inputCarriage == 4)
    {
      // ожидаем нажатия символа "#" на клавиатуре и сравниваем введённый код с установленым
      if (GetKeyPressed() == 11 && GetCodeFromInput(inputFromKeypad) == securityCode)
      {
        // в случае правильного ввода включаем пьезодинамик на 250 мс, переключаем состояние реле и сбрасываем ввод
        buzer = 1;
        wait_ms(250);
        buzer = 0;
        isSecurityEnabled = !isSecurityEnabled;
        strcpy(inputFromKeypad, "####");
        inputCarriage = 0;
        relay = isSecurityEnabled ? 1 : 0;
      }
      else if (GetKeyPressed() == 11 && GetCodeFromInput(inputFromKeypad) != securityCode)
      {
        // в случае неправильного ввода включаем пьезодинамик на 500 мс и сбрасываем ввод
        buzer = 1;
        wait_ms(500);
        buzer = 0;
        strcpy(inputFromKeypad, "####");
        inputCarriage = 0;
      }
    }

    // проверяем включенность режима "Объект под охраной"
    if (isSecurityEnabled)
    {
      // выводим состояние датчиков на светодиоды, если режим включен
      led_orange = zone_1 ? 0 : 1;
      led_green = zone_2 ? 0 : 1;
      led_red = zone_3 ? 0 : 1;
      led_blue = zone_4 ? 0 : 1;

      // при разрыве в цепи одного из датчиков - отправляем данные о датчиках через последовательный порт на Wi-Fi модуль
      if (zone_1 || zone_2 || zone_3 || zone_4)
      {
        wi_fi.puts("AZ|" + zone_1 + zone_2 + zone_3 + zone_4);
      }
    }
    else
    {
      // "гасим" светодиоды, если режим выключен
      led_orange = 0;
      led_green = 0;
      led_red = 0;
      led_blue = 0;
    }
  }
}
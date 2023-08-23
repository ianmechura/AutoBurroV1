//Input and processing related constants.
#define X_AXIS_IN_PIN A1
#define Y_AXIS_IN_PIN A0

#define DEADBAND 50
#define X_AXIS_CENTER 513
#define Y_AXIS_CENTER 518

#define TOP 255
#define CENTER 127
#define BOTTOM 0
#define BIT8_DEADBAND 15

//Output related constants.
#define RIGHT_DIR_OUT_PIN 2
#define LEFT_DIR_OUT_PIN 3

//PWM output pins
#define RIGHT_PWR_OUT_PIN 9
#define LEFT_PWR_OUT_PIN 10

int x_in, y_in, x, y = 0;
int r_dir, l_dir, r_pwr, l_pwr;

int r_limit = -1;
int l_limit = -1;

int debug_out = 0;

class TTYTerminal {
  public:
    int cmd_i = 0;
    int input_rec = 0;
    char command[128];
    char tty_command[128];

    void init() {
      Serial.println(F("TTYTerminal Initialized"));
      Serial.println(F("Enter $help for command listing"));
    }

    void receive() {
      char input;

      while (Serial.available()) {
        input = (char)Serial.read();
        if (input != '\n') {
          command[cmd_i] = input;
          cmd_i++;
          command[cmd_i] = '\0';
        } else {
          sprintf(tty_command, "%s", command);
          cmd_i = 0;
          sprintf(command, "");
          input_rec = 1;
        }
      }
    }

    void process_event(char * tty_command) {
      if ( strcmp( tty_command, "" ) == 0 ) {
        return;
      }
      char * command = strtok(tty_command, "=");
      char * argument = strtok(0, "=");
      char buff[80];

      strcpy(buff, "");

      // $set-right-limit=100
      // $set-left-limit=100
      // $get-right_limit
      // $get-left-limit

      if (strcmp( command, "$set-debug") == 0) {
        debug_out = atoi(argument);
        sprintf(buff, "[DEBUG]: %d", debug_out);
      } else if (strcmp( command, "$set-right-limit") == 0) {
        r_limit = atoi(argument);
        sprintf(buff, "[RIGHT_LIMIT]: %d", r_limit);
      } else if (strcmp( command, "$set-left-limit") == 0) {
        l_limit = atoi(argument);
        sprintf(buff, "[LEFT_LIMIT]: %d", l_limit);
      } else if (strcmp( command, "$get-right-limit") == 0) {
        sprintf(buff, "[RIGHT_LIMIT]: %d", r_limit);
      } else if (strcmp( command, "$get-left-limit") == 0) {
        sprintf(buff, "[LEFT_LIMIT]: %d", l_limit);
      } else {
        sprintf(buff, "Invalid command: [%s]", tty_command);
      }

      Serial.println(buff);
      strcpy(tty_command, "");
    }

    void process() {
      if (this->input_rec == 1) {
        this->input_rec = 0;
        this->process_event(this->tty_command);
      }
    }
};

TTYTerminal terminal;

void serialEvent() {
  terminal.receive();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Serial Iniitialized.");

  pinMode(RIGHT_DIR_OUT_PIN, OUTPUT);
  digitalWrite(RIGHT_DIR_OUT_PIN, 1);
  pinMode(LEFT_DIR_OUT_PIN, OUTPUT);
  digitalWrite(LEFT_DIR_OUT_PIN, 1);

  pinMode(X_AXIS_IN_PIN, INPUT);
  pinMode(Y_AXIS_IN_PIN, INPUT);

  Serial.println("Setup complete.");

}

void motor_right(int r_dir, int r_pwr) {
  digitalWrite(RIGHT_DIR_OUT_PIN, r_dir);
  analogWrite(RIGHT_PWR_OUT_PIN, r_pwr);
}

void motor_left(int l_dir, int l_pwr) {
  digitalWrite(LEFT_DIR_OUT_PIN, l_dir);
  analogWrite(LEFT_PWR_OUT_PIN, l_pwr);
}

int is_deadband(int center, int input) {
  return  ( (input >= (center - BIT8_DEADBAND)) && (input <= (center + BIT8_DEADBAND)) );
}

void loop() {
  terminal.process();

  //sample joystick potentiometers
  x_in = analogRead(X_AXIS_IN_PIN);
  y_in = analogRead(Y_AXIS_IN_PIN);

  //pots are returning a digital voltage signal that is 0 - 1024.
  //it needs to be converted to (-512) - (+512)
  x = X_AXIS_CENTER - x_in;
  y = Y_AXIS_CENTER - y_in;

  r_pwr = y - x;  // mix x and y to determine motor voltage
  l_pwr = y + x;  // ...

  //if its a negative value then it is clockwise, positive is counter clockwise
  r_dir = r_pwr <= 0;
  l_dir = l_pwr <= 0;

  //map back from (-512) - (+512) to 0 - 255
  r_pwr = abs(map(r_pwr, BOTTOM, TOP, 0, 255));
  l_pwr = abs(map(l_pwr, BOTTOM, TOP, 0, 255));

  //if a limit has been set then apply the limit to the motor output.
  if (r_limit != -1) {
    r_pwr  = (r_pwr <= r_limit ) ? r_pwr : r_limit;
    if (debug_out) {
      char b[32];
      sprintf(b, "Limiting right motor power to %d", r_limit);
    }
  }

  if (l_limit != -1) {
    l_pwr = (l_pwr <= l_limit) ? l_pwr : l_limit;
    if (debug_out) {
      char b[32];
      sprintf(b, "Limiting left motor power to %d", r_limit);
    }
  }

  if ( is_deadband( CENTER, r_pwr ) && is_deadband( CENTER, l_pwr) ) {
    //if the stick is in its normal position (accounting for deadband) then stop movement
    motor_right(0, 0);
    motor_left(0, 0);
  } else {
    //write out pin values for motor speed and directional control
    motor_right(r_dir, r_pwr);
    motor_left(l_dir, l_pwr);
  }

  if (debug_out) {
    char b[32];
    sprintf(b, "%d, %d, %d, %d", l_pwr, l_dir, r_pwr, r_dir);
    Serial.println(b);
  }

}

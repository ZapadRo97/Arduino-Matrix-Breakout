/**********************************
Arduino Breakout Game
Made using 8x8 LED Matrix Display and MAX7219
Made by Purcea Florin Daniel
Email: zapad.daniel7@gmail.com

Copyright (C) 2017 Purcea Florin 
**********************************/

#include <LiquidCrystal.h>
#include "LedControl.h" //  need the library

//8X8 MATRIX INIT
const int din = 12, clk = 11, load = 10;
LedControl lc = LedControl(din, clk, load, 1); // 1 as we are only using 1 MAX7219

//LCD DISPLAY INIT
const int rs = 9, en = 8, d4 = 4, d5 = 2, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int lcd_contrast = 3;
const int button_pin = 13;

const int buzzer = 5;

const int joy_x = A0;
int joy_value_x = 0;

unsigned long current_time = 0, previous_time = 0, previous_time_ball = 0;

int count_bits(byte byte_row)
{
  int count = 0;
  while (byte_row != 0)
  {
    byte_row = byte_row & (byte_row - 1);
    count++;
  }
  return count;
}

//Level class
class Level
{
  private:
    byte levels[4];
    bool started;

  public:
    Level(int nr_level);
    void load_level();
    void start_level();
    void stop_level();
    bool is_started();
    bool bricks_left();
    byte get_level_row(int row);
    void destroy_brick(int row, int column);
};

Level::Level(int nr_level)
{
  started = false;
  switch (nr_level)
  {
    case 1:
      {
        levels[0] = B00111100;
        levels[1] = B01111110;
        levels[2] = B11111111;
        levels[3] = B00000000;
        break;
      }
    case 2:
      {
        levels[0] = B01100110;
        levels[1] = B10111101;
        levels[2] = B01111110;
        levels[3] = B00000000;
        break;
      }
    case 3:
      {
        levels[0] = B01111110;
        levels[1] = B11000011;
        levels[2] = B11111111;
        levels[3] = B00000000;
        break;
      }
  }
}

void Level::load_level()
{
  for (int i = 0; i < 4; i++)
  {
    lc.setRow(0, i, levels[i]);
  }
}

void Level::start_level()
{
  started = true;
}

void Level::stop_level()
{
  started = false;
}

bool Level::is_started()
{
  return started;
}

bool Level::bricks_left()
{
  int count = 0;
  for (short i = 0; i < 4; i++)
  {
    byte row = levels[i];
    count += count_bits(row);
  }
  return count > 0 ? true : false;
}

byte Level::get_level_row(int row)
{
  return levels[row];
}

void Level::destroy_brick(int row, int column)
{
  levels[row] = (~(1 << column)) & levels[row];
}
///////////////////

//Ball class
class Ball
{
  private:
    unsigned int ball_speed;
    int ball_position[2];
    int ball_move[2];

  public:
    Ball();
    void increase_speed();
    int get_speed();
    void reset_speed();
    void move_ball(int x, int y); //this time, x and y are justified
    int get_x_coord();
    int get_y_coord();
    int get_x_move();
    int get_y_move();

};
Ball::Ball()
{
  ball_speed = 1;
  ball_position[0] = 6;
  ball_position[1] = 4;

  ball_move[0] = 0;
  ball_move[1] = 0;
  lc.setLed(0, ball_position[0], ball_position[1], true);
}

void Ball::move_ball(int x, int y)
{
  int temp_y = ball_position[0] + y;
  int temp_x = ball_position[1] + x;
  if (temp_x < 0 || temp_x > 7)
    return;
  if (temp_y < 0 || temp_y > 7)
    return;


  ball_move[0] = y;
  ball_move[1] = x;

  lc.setLed(0, ball_position[0], ball_position[1], false);
  ball_position[0] += y;
  ball_position[1] += x;
  lc.setLed(0, ball_position[0], ball_position[1], true);
}

int Ball::get_x_coord()
{
  return ball_position[1];
}

int Ball::get_y_coord()
{
  return ball_position[0];
}

int Ball::get_x_move()
{
  return ball_move[1];
}
int Ball::get_y_move()
{
  return ball_move[0];
}

void Ball::increase_speed()
{
  ball_speed++;
}
void Ball::reset_speed()
{
  ball_speed = 1;
}
int Ball::get_speed()
{
  return ball_speed;
}
///////////////////

//Platform class
class Platform
{
  private:
    byte platform;

  public:
    Platform();
    void move_platform(int coord);
    byte get_platform();
};

Platform::Platform()
{
  platform = B00011100;
  lc.setRow(0, 7, platform);
}

byte Platform::get_platform()
{
  return platform;
}

void Platform::move_platform(int coord)
{
  if (coord == 2)
    return;

  if (platform > 7 && platform < 224)
  {
    if (coord > 0)
      platform = platform >> coord;
    else
      platform = platform << (-coord);
  }
  else if (platform == 7) //far left
  {
    if (coord < 0)
    {
      Serial.println("Platform move left");
      platform = platform << (-coord);
      //platform = platform | 1;
    }
  }
  else if (platform == 224) //far right
  {
    if (coord > 0)
    {
      Serial.println("Platform move right");
      platform = platform >> coord;
      //platform = platform | 128;
    }

  }

  lc.setRow(0, 7, platform);
}
//////////////////

//Game class
class Game
{
  private:
    bool started;
    int number_of_lives;
    int score;
    int level;

    Level *level_obj;
    Platform *platform_obj;
    Ball *ball_obj;

  public:
    Game();
    void start_game();
    void stop_game();
    void restart_level();
    bool is_started();
    Platform* get_platform();
    Level* get_level();
    Ball* get_ball();
    int get_level_number();
    void move_ball_calculated();
    void lose_live();
    int get_lives();
    void next_level();
    int get_score();
};

Game::Game()
{
  level = 1;
  number_of_lives = 3;
  score = 0;
  started = false;

  lc.clearDisplay(0);

  lcd.clear();
  lcd.print("PRESS BUTTON");
  lcd.setCursor(0, 1);
  lcd.print("TO START");

  level_obj = NULL;
  platform_obj = NULL;
  ball_obj = NULL;
}

bool Game::is_started()
{
  return started;
}

int Game::get_score()
{
  return score;
}

Platform* Game::get_platform()
{
  return platform_obj;
}

Level* Game::get_level()
{
  return level_obj;
}

int Game::get_level_number()
{
  return level;
}

Ball* Game::get_ball()
{
  return ball_obj;
}

void Game::start_game()
{
  started = true;

  //lcd stuff
  lcd.clear();
  lcd.print("Level:");
  lcd.print(level);
  lcd.print(" ");
  lcd.print("Lives:");
  lcd.print(number_of_lives);
  lcd.setCursor(0, 1);
  lcd.print("Score:");
  lcd.print(score);


  if (level < 7)
  {
    Serial.println("Loading level");
    level_obj = new Level(level);
    level_obj->load_level();

    Serial.println("Loading platform");
    platform_obj = new Platform();

    Serial.println("Loading ball");
    ball_obj = new Ball();

  }

}

void Game::restart_level()
{
  delete platform_obj;
  delete ball_obj;

  Serial.println("Loading platform");
  platform_obj = new Platform();

  Serial.println("Loading ball");
  ball_obj = new Ball();

}

void Game:: next_level()
{
  level++;
  number_of_lives++;
  lcd.clear();
  lc.clearDisplay(0);
  lcd.print("PRESS BUTTON");
  lcd.setCursor(0, 1);
  lcd.print("FOR NEXT LEVEL");

  score += level * 10;

}

void Game::stop_game()
{
  delete level_obj;
  delete platform_obj;
  delete ball_obj;

  started = false;
}

void Game::lose_live()
{
  number_of_lives--;
  lcd.setCursor(14, 0);
  lcd.print(number_of_lives);
}

int Game::get_lives()
{
  return number_of_lives;
}

void Game::move_ball_calculated()
{
  //is ball touching the platform?
  if (ball_obj->get_y_coord() == 6 && ( ((1 << (7 - ball_obj->get_x_coord())) & (platform_obj->get_platform())) != 0))
  {
    tone(buzzer, 450, 100);
    Serial.println("Is touching platform");
    //if yes, where?
    byte ball_row = 255 >> (ball_obj->get_x_coord());
    byte ball_row_and_platform = ball_row & platform_obj->get_platform();

    short position = count_bits(ball_row_and_platform);

    if (ball_obj->get_x_move() == 0 && (ball_obj->get_x_coord() == 0 || ball_obj->get_x_coord() == 7))
    {
      Serial.println("For start far left/right");
      ball_obj->move_ball(position - 2, -1);
    }
    else if (ball_obj->get_x_move() == 0)
    {
      Serial.println("For start");
      ball_obj->move_ball(2 - position, -1);
    }
    else if (position != 2 && (ball_obj->get_x_coord() == 0 || ball_obj->get_x_coord() == 7))
    {
      Serial.println("Not mid and far left/right");
      ball_obj->move_ball(-(ball_obj->get_x_move()), -1);
    }
    else if (position != 2)
    {
      Serial.println("Not mid");
      ball_obj->move_ball(ball_obj->get_x_move(), -1);
    }
    else if (position == 2)
    {
      Serial.println("Mid");
      ball_obj->move_ball(0, -1);
    }


  }

  //Special case for platform edges
  else if (ball_obj->get_y_coord() == 6 && ( ((1 << (6 - ball_obj->get_x_coord())) & (platform_obj->get_platform())) != 0) && ball_obj->get_x_move() == 1)
  {
    tone(buzzer, 450, 100);
    Serial.println("Right EDGE");
    ball_obj->move_ball(1, 0);
    delay(40);
    ball_obj->move_ball(-1, -1);
  }
  else if (ball_obj->get_y_coord() == 6 && ( ((1 << (8 - ball_obj->get_x_coord())) & (platform_obj->get_platform())) != 0) && ball_obj->get_x_move() == -1)
  {
    tone(buzzer, 450, 100);
    Serial.println("Left EDGE");
    ball_obj->move_ball(-1, 0);
    delay(40);
    ball_obj->move_ball(1, -1);
  }

  //is ball touching a brick?
  else if ((((1 << (7 - ball_obj->get_x_coord())) & (level_obj->get_level_row(ball_obj->get_y_coord()))) != 0) && ball_obj->get_y_coord() < 4)
  {
    ball_obj->increase_speed();
    score += (ball_obj->get_speed()) * 10;
    lcd.setCursor(6, 1);
    lcd.print(score);

    Serial.println("Brick contact");
    level_obj->destroy_brick(ball_obj->get_y_coord(), 7 - ball_obj->get_x_coord());
    ball_obj->move_ball(-(ball_obj->get_x_move()), -(ball_obj->get_y_move()));
    tone(buzzer, 1350, 100);
  }

  //is ball on lowest row?
  else if (ball_obj->get_y_coord() == 7)
  {
    Serial.println("Lose one live");
    lose_live();

  }

  //is ball on corner
  else if (ball_obj->get_y_coord() == 0 && (ball_obj->get_x_coord() == 0 || ball_obj->get_x_coord() == 7))
    ball_obj->move_ball(-(ball_obj->get_x_move()), -(ball_obj->get_y_move()));

  //is ball touching a wall?
  else if (ball_obj->get_x_coord() == 0 || ball_obj->get_x_coord() == 7)
  {
    Serial.println("Right/Left wall");
    ball_obj->move_ball(-(ball_obj->get_x_move()), ball_obj->get_y_move());
  }
  else if (ball_obj->get_y_coord() == 0 )
  {
    Serial.println("TOP wall");
    ball_obj->move_ball(ball_obj->get_x_move(), -(ball_obj->get_y_move()));
  }



  //is ball in mid air?
  else
  {
    //Serial.println("Is in mid air");
    ball_obj->move_ball(ball_obj->get_x_move(), ball_obj->get_y_move());
  }
}
////////////////

void setup()
{
  lc.shutdown(0, false); // turn off power saving, enables display
  lc.setIntensity(0, 8); // sets brightness (0~15 possible values)
  lc.clearDisplay(0);// clear screen

  pinMode(lcd_contrast, OUTPUT);
  analogWrite(lcd_contrast, 102); //we need 2v on vo (255 = 5v)

  Serial.begin(9600);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.


  pinMode(button_pin, INPUT);
  digitalWrite(button_pin, HIGH);

  //experimental stuff
  pinMode(buzzer, OUTPUT);
}

Game *game_inst = NULL;
int platform_move_interval = 100;
int ball_move_interval = 255;

int current_lives = 0;

void loop()
{
  analogWrite(lcd_contrast, 102); //dirty hack
  if (game_inst == NULL)
  {
    Serial.println("NEW GAME");
    game_inst = new Game();
  }

  if (!game_inst->is_started())
  {
    short int push_button = digitalRead(button_pin);
    if (push_button == LOW)
    {


      game_inst->start_game();
      current_lives = game_inst->get_lives();
      delay(500);
    }
  }
  else //Game is started
  {

    //check for lives left
    //some lives left
    if (game_inst->get_lives() != current_lives)
    {
      Serial.println("RESTART LEVEL");
      current_lives = game_inst->get_lives();
      game_inst->get_level()->stop_level();
      game_inst->restart_level();
    }

    //no lives left
    if (game_inst->get_lives() == 0)
    {
      Serial.println("GAME OVER");
      game_inst->stop_game();
      delay(100);
      delete game_inst;
      game_inst = NULL;
    }

    //win game


    current_time = millis();

    if (current_time - previous_time > platform_move_interval)
    {
      previous_time = current_time;
      joy_value_x = analogRead(joy_x);
      int temp = map(joy_value_x, 0, 1023, -1, 2);
      game_inst->get_platform()->move_platform(temp);
      //level is not started
      if (!game_inst->get_level()->is_started())
      {
        if (temp < 2)
        {
          game_inst->get_ball()->move_ball(temp, 0);
        }
      }
    }

    if (!game_inst->get_level()->is_started())
    {
      short int push_button = digitalRead(button_pin);
      if (push_button == LOW)
        game_inst->get_level()->start_level();
    }
    else //Level is started
    {
      if (game_inst->get_level()->bricks_left())
      {
        if (current_time - previous_time_ball > (ball_move_interval - game_inst->get_ball()->get_speed() * 5))
        {
          previous_time_ball = current_time;
          game_inst->move_ball_calculated();
        }
      }
      else
      {
        if (game_inst->get_level_number() == 3)
        {
          Serial.println("Win GAME");
          lc.clearDisplay(0);
          lcd.clear();
          lcd.print("WIN GAME");
          lcd.setCursor(0, 1);
          lcd.print("Score:");
          lcd.print(game_inst->get_score());

          game_inst->stop_game();
          delete game_inst;
          game_inst = NULL;
          delay(10000);

        }
        else
        {
          Serial.println("GO TO NEXT LEVEL");
          game_inst->stop_game();
          delay(100);
          game_inst->next_level();
          //game_inst->start_game();
        }


      }
    }

  }

}

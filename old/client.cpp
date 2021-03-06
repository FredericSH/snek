#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>


enum Direction {UP,RIGHT,DOWN,LEFT};
enum Event {MOVEDUP,MOVEDRIGHT,MOVEDOWN,MOVEDLEFT,MOVEDOUTOF,MOVEDINTO};
const int snakeSpeed = 1;
const int snakeWidth = 1;

// standard U of A library settings, assuming Atmel Mega SPI pins
#define SD_CS    5  // Chip select line for SD card
#define TFT_CS   6  // Chip select line for TFT display
#define TFT_DC   7  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

const int VERT = 0;
const int HOR = 1;
const int SEL = 9;

class JoystickListener{
  private:
    int horizontalPin;
    int verticalPin;
    int depressionPin;
    int verticalBaseline;
    int horizontalBaseline;
    int threshold;
  public:
    // Pass the joystickListener the pin numbers that the joystick is hooked up to as well as the minimum threshold for input.
    JoystickListener(int vert, int hort, int sel, int thresh){
      verticalPin = vert;
      horizontalPin = hort;
      depressionPin = sel; 
      digitalWrite(depressionPin, HIGH);
      verticalBaseline = analogRead(verticalPin);
      horizontalBaseline = analogRead(horizontalPin);
      threshold = thresh;
      
    }
    //Returns the value of the vertical pin
   int getVertical(){
     return analogRead(verticalPin);
   }
   //Returns the original baseline of the vertical pin
   int getVerticalBaseline(){
     return verticalBaseline;
   }
   //Returns the value of the horizontal pin
   int getHorizontal(){
     return analogRead(horizontalPin); 
   }
   //Returs the original baseline of the horizontal pin
   int getHorizontalBaseline(){
     return horizontalBaseline; 
   }   
   //Returns the state of the Joystick's button
   boolean isDepressed(){
     return !digitalRead(depressionPin); 
   }
   //Returns true if the joystick is pushed horizontally past the minimum threshold, false otherwise
   boolean isPushedHorizontally(){
     return (abs(horizontalBaseline - getHorizontal()) > threshold);
   }
   //Returns true if the joystick is pushed vertically past the minimum threshold, false otherwise
   boolean isPushedVertically(){
     return (abs(verticalBaseline - getVertical()) > threshold);
   }
   //Returns true if the joystick is pushed vertically or horiztonally, false if not pushed past either threshold.
   boolean isPushed(){
     return isPushedVertically() || isPushedHorizontally();
   } 
};

class Snake{
  private:
  class SnakeComponent{
    private:
    uint8_t x;
    uint8_t y;
    uint16_t colour;
    Direction dir;
    int layer;
    public:
    SnakeComponent(uint8_t startX,uint8_t startY,Direction startDir,uint16_t col){
      x = startX;
      y = startY;
      dir = startDir;
      colour = col;
      layer = 1;
    }  
    void update(){
      x -= (dir%2 == 1) ? dir - 2 : 0;  
      y += (dir%2 == 0) ? dir - 1 : 0;
      if(x == 128) x = 0;
      if(x == 255) x = 128;
      if(y == 160) y = 0;
      if(y == 255) y = 159;
      if(layer == 0){
        tft.drawPixel(x,y,colour);
      }
    }
    void setDirection(Direction newDirection){
       dir = newDirection;  
    }
    Direction getDirection(){
      return dir;  
    }
    uint8_t getX(){
      return x;  
    }
    uint8_t getY(){
      return y;  
    }
    void setX(uint8_t newX){
      x = newX;  
    }
    void setY(uint8_t newY){
      y = newY;  
    }
    int getLayer(){
      return layer; 
    }
    void setLayer(int newLayer){
      layer = newLayer;  
    }
  };
  struct snakeEvent{
      uint8_t x;
      uint8_t y;
      Event e;
  };
  snakeEvent eventQueue[128];
  uint8_t queueIndex;
  int activeEventIndex;
  int pendingLength;
  SnakeComponent* headComp;
  SnakeComponent* tailComp;
  boolean dead;
  int number;
  public:
  Snake(uint8_t startX, uint8_t startY,Direction startDir, uint16_t col, int startingLength,int num) :
    headComp(new SnakeComponent(startX,startY,startDir,col)) , 
    tailComp(new SnakeComponent(startX,startY,startDir,0x0)) ,
    pendingLength(startingLength),
    queueIndex(0),
    activeEventIndex(0),
    dead(false),
    number(num)
    {
    }
  void update(){
    headComp->update();
    if(pendingLength != 0){
        pendingLength--;
    }
    else{
      if(activeEventIndex != queueIndex && tailComp->getX() == eventQueue[activeEventIndex].x && tailComp->getY() == eventQueue[activeEventIndex].y){
         if(eventQueue[activeEventIndex].e == 4){
          tailComp->setLayer(1);
          activeEventIndex++;
         }
         else if(eventQueue[activeEventIndex].e == 5){
          tailComp->setLayer(0);
          activeEventIndex++;
         }
         else{
          tailComp->setDirection((Direction)(eventQueue[activeEventIndex++].e));
         }
         if(activeEventIndex == 128) activeEventIndex = 0; 
      }
      tailComp->update();  
    }
  }
  void addEvent(struct snakeEvent e){
    eventQueue[queueIndex++] = e; 
    if(queueIndex == 128)queueIndex = 0;
  }
  void generateEvent(int event){
    snakeEvent temp;
    temp.x = headComp->getX();
    temp.y = headComp->getY();
    temp.e = (Event) event;
    addEvent(temp);
  }
  void setDirection(Direction newDirection){
          headComp->setDirection(newDirection);
          generateEvent(newDirection);
  }
  void setDirection(int d){
    setDirection((d == 0) ? UP : (d == 1) ? RIGHT : (d == 2) ? DOWN : LEFT);  
  }
  boolean isDead(){
      return dead;  
  }
  void setDead(boolean b){
    dead = b;  
  }
  uint8_t getX(){
    return headComp->getX();  
  }
  uint8_t getY(){
    return headComp->getY();  
  }
  void setX(uint8_t x){
    headComp->setX(x);
  }
  void setY(uint8_t y){
    headComp->setY(y); 
  }
  uint8_t getTailX(){
    return tailComp->getX();  
  }
  uint8_t getTailY(){
    return tailComp->getY();  
  }
  int getLayer(){
    return headComp->getLayer();  
  }
  int getTailLayer(){
    return tailComp->getLayer();
  }
  void setLayer(int newLayer){
    if(getLayer() != newLayer){
      headComp->setLayer(newLayer);
      if(newLayer == 0){
        generateEvent(4);
      }
      else{
        generateEvent(5);  
      }
    }  
  }
  void writeSerializedSnake(){
    Serial2.write(headComp->getX());
    Serial2.write(headComp->getY());
    Serial2.write(headComp->getDirection());  
    Serial2.write(dead);
    Serial2.write(number);
  }
};

const int fps = 24;


typedef struct{
  uint8_t pixel[16][160];
}board;
class GameManager{
  private:
    Snake* s[3];
    JoystickListener* js;
  public:    
    GameManager() :js(new JoystickListener(VERT,HOR,SEL,450)){
      tft.fillScreen(0);
      s[0] = new Snake(20,20,DOWN,0xFF00,20,1);
      s[1] = new Snake(20,100,RIGHT,0x0FF0,20,2);
      s[2] = new Snake(100,108,UP,0x00FF,20,3); 
      while(!Serial2.available());
      parseServerPacket(true);
      parseServerPacket(true);
      parseServerPacket(true);            
    }
    void parseServerPacket(boolean initializing){
      uint8_t x = Serial2.read();
      uint8_t y = Serial2.read();
      uint8_t dir = Serial2.read();
      uint8_t layer = Serial2.read();
      boolean isDead = Serial2.read();
      uint8_t snakeNum = Serial2.read();
      
      if(initializing){
          s[snakeNum]->setX(x);
          s[snakeNum]->setY(y);
          s[snakeNum]->setDirection(dir);  
      }
      else if(s[snakeNum]->isDead() != isDead){
        s[snakeNum]->setDead(true); 
      }
      else{
        if(layer == 0){
          s[snakeNum]->setDirection(dir);
        }  
        else{
          s[snakeNum]->setLayer(1); 
       }
      }
    }
    void run(){
      uint32_t time = millis();
      while(!(s[0]->isDead()&&s[1]->isDead()&&s[2]->isDead())){
        if(millis() - time > 1000/fps){
          time = millis();
          for(int i = 0; i < 3; i++){
            if(s[i]->isDead())continue;
            s[i]->update();
          }
          if(js->isPushed()){
            int deltaH = js->getHorizontal() - js->getHorizontalBaseline();
            int deltaV = js->getVertical() - js->getVerticalBaseline();
            if(abs(deltaH) > abs(deltaV)){
              (deltaH > 0) ? Serial2.write(1) : Serial2.write(3);    
            }
            else{
              (deltaV > 0) ? Serial2.write(2) : Serial2.write(UP);
            }
          }
          else if(js->isDepressed()){
            Serial2.write(4);
          }
          if(Serial2.available()){
            parseServerPacket(false);  
          }
        }
      }  
    }
  
};
int main(){
  init();
  tft.initR(INITR_REDTAB); // initialize a ST7735R chip, green tab
  Serial.begin(9600);
  GameManager* gm = new GameManager();
  gm->run();
  Serial.end();
  return 0;
};

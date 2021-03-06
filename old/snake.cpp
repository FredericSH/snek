#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>


enum Direction {UP,RIGHT,DOWN,LEFT};
enum Event {MOVEDUP,MOVEDRIGHT,MOVEDOWN,MOVEDLEFT,MOVEDHIGHER,MOVEDLOWER};
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

struct snakeEvent{
      uint8_t x;
      uint8_t y;
      uint8_t layer;
      Event e;
  };

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
    uint8_t layer;
    uint16_t colour;
    Direction dir;
    public:
    SnakeComponent(uint8_t startX,uint8_t startY,Direction startDir,uint16_t col){
      x = startX;
      y = startY;
      dir = startDir;
      colour = col;
      layer = 0;
    }  
    void update(){
      x -= (dir%2 == 1) ? dir - 2 : 0;
      if(x == 255) x = 127;
      if(x == 128) x = 0; 
      y += (dir%2 == 0) ? dir - 1 : 0;
      if(y == 255) y = 159;
      if(y == 160) y = 0;
      if(layer == 0)tft.drawPixel(x,y,colour);
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
    void setLayer(int l){
      layer = l;
    }
    uint8_t getLayer(){
      return layer;
    }
  };
  snakeEvent eventQueue[128];
  uint8_t queueIndex;
  int pendingLength;
  int layer;
  int activeEventIndex;
  SnakeComponent* headComp;
  SnakeComponent* tailComp;
  boolean dead;
  public:
  Snake(uint8_t startX, uint8_t startY,Direction startDir, uint16_t col, int startingLength) :
    headComp(new SnakeComponent(startX,startY,startDir,col)) , 
    tailComp(new SnakeComponent(startX,startY,startDir,0x0)) ,
    pendingLength(startingLength),
    layer(0),
    queueIndex(0),
    activeEventIndex(0),
    dead(false)
    {
    }
  void update(){
    headComp->update();
    if(pendingLength != 0){
        pendingLength--;
    }
    else{
      if(activeEventIndex!=queueIndex && tailComp->getX() == eventQueue[activeEventIndex].x && tailComp->getY() == eventQueue[activeEventIndex].y){
        tailComp->setDirection((Direction)(eventQueue[activeEventIndex++].e));
        tailComp->setLayer(eventQueue[activeEventIndex-1].layer);
         if(activeEventIndex == 128) activeEventIndex = 0; 
      }
      tailComp->update();  
    }
  }
  void setDirection(Direction newDirection){
      if(newDirection%2 != headComp->getDirection()%2){
          headComp->setDirection(newDirection);
          snakeEvent temp;
          temp.x = headComp->getX();
          temp.y = headComp->getY();
          temp.e = (Event) newDirection;
          temp.layer = headComp->getLayer();
          eventQueue[queueIndex++] = temp; 
          if(queueIndex == 128)queueIndex = 0;
      }   
  }
  uint8_t getX(){
    return headComp->getX();
  }
  uint8_t getY(){
    return headComp->getY();
  }
  uint8_t getTailX(){
    return tailComp->getX();
  }
  uint8_t getTailY(){
    return tailComp->getY();
  }
  uint8_t getTailLayer(){
    return tailComp->getLayer();
  }
  Direction getDirection(){
    return headComp->getDirection();
  }
  void setDirection(int d){
    setDirection((d == 0) ? UP : (d == 1) ? RIGHT : (d == 2) ? DOWN : LEFT);  
  }
  void kill(){
    dead = true;
  }
  boolean isDead(){
      return dead;  
  }
  void setLayer(){
    int newLayer = (layer == 1) ? 0 : 1;
    layer = newLayer;
    headComp->setLayer(newLayer);
    snakeEvent temp;
    temp.x = headComp->getX();
    temp.y = headComp->getY();
    temp.e = (Event)headComp->getDirection();
    temp.layer = newLayer;
  }
  int getLayer(){
    return layer;
  }
  uint8_t getQIndex(){
    return queueIndex;
  }
  uint8_t getEIndex(){
    return activeEventIndex;
  }
  snakeEvent getEvent(int index){
    return eventQueue[index];
  }
};

const int fps = 60;


//Receive changes in direction from the clients
//Send to clients if stuff has to be drawn on their screen 
//Changes in direction to snakes on clients screen

class GameManager{
  private:
    Snake* s[3];
    JoystickListener* js;
    void parseClientPacket(uint8_t packet, Snake* s){
      if(packet > 3){
        if(packet == 4){
          s->setLayer();
          
        }else if(packet ==5){
          s->setLayer();
          
        }
      }
      else{
        s->setDirection(packet);
      }
    }
  public:    
    GameManager() : js (new JoystickListener(VERT,HOR,SEL,450)){
      tft.fillScreen(0);
      s[0] = new Snake(20,20,DOWN,0xFF00,20);
      s[1] = new Snake(20,100,RIGHT,0x0FF0,20);
      s[2] = new Snake(100,108,UP,0x00FF,20);
    }
    bool intersects(int X, int Y, int x1, int y1, int x2, int y2){
      if(x1 == x2){
        return(X == x1 && Y > min(y1,y2) && Y < max(y1,y2));
      }else{
        return(Y == y1 && X > min(x1,x2) && X < max(x1,x2));
      }
    }
    void detectCollision(){
      int qindex, eindex;
      for(int i = 0; i < 3; i++){
        if(s[i]->isDead())continue;
        
        for(int j = 0; j < 3; j++){
          qindex = s[j]->getQIndex();
          eindex = s[j]->getEIndex();
          if(qindex == eindex){
            if(s[i]->getLayer() == s[j]->getTailLayer() && intersects(s[i]->getX(),s[i]->getY(),s[j]->getX(),s[j]->getY(),s[j]->getTailX(),s[j]->getTailY())){
              s[i]->kill();
              break;
            }
            continue;
          }else{
            if(s[i]->getLayer() == s[j]->getEvent(qindex-1).layer && intersects(s[i]->getX(),s[i]->getY(),s[j]->getX(),s[j]->getY(),s[j]->getEvent(qindex-1).x,s[j]->getEvent(qindex-1).y)){
              s[i]->kill();
              break;
            }
            if(s[i]->getLayer() == s[j]->getTailLayer() && intersects(s[i]->getX(),s[i]->getY(),s[j]->getEvent(eindex).x,s[j]->getEvent(eindex).y,s[j]->getTailX(),s[j]->getTailY())){
              s[i]->kill();
              break;
            }
          }
          qindex = (qindex+127)%128;
          while(qindex!=eindex){
            if(s[i]->getLayer() == s[j]->getEvent((qindex+127)%128).layer && intersects(s[i]->getX(),s[i]->getY(),s[j]->getEvent(qindex).x,s[j]->getEvent(qindex).y,s[j]->getEvent((qindex+127)%128).x,s[j]->getEvent((qindex+127)%128).y)){
              s[i]->kill();
              break;
            }
            qindex = (qindex+127)%128;
          }
        }
      }
    }
    void run(){
      uint32_t time = millis();
      bool handled = 0;
      while(!(s[0]->isDead() && s[1]->isDead() && s[2]->isDead())){
        if(millis() - time > 1000/fps){
          time = millis();
          for(int i = 0; i < 3; i++){
            if(s[i]->isDead())continue;
            s[i]->update();
          }
          detectCollision();
          if(js->isPushed()){
            int deltaH = js->getHorizontal() - js->getHorizontalBaseline();
            int deltaV = js->getVertical() - js->getVerticalBaseline();
            if(abs(deltaH) > abs(deltaV)){
              s[0]->setDirection((deltaH > 0) ? 1 : 3);    
            }
            else{
              s[0]->setDirection((deltaV > 0) ? 2 : 0);
            }
          }
          if(js->isDepressed()){
            if(!handled){
              s[0]->setLayer();
              handled = 1;
            }
          }else{
            handled = 0;
          }
          if(Serial2.available()){
            parseClientPacket(Serial2.read(),s[1]);
          }
          if(Serial3.available()){
            parseClientPacket(Serial3.read(),s[2]);
          }
        }
      }  
    }
};
int main(){
  init();
  tft.initR(INITR_REDTAB); // initialize a ST7735R chip, green tab
  Serial.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(9600);
  GameManager* gm = new GameManager();
  gm->run();
  Serial.end();
  return 0;
}

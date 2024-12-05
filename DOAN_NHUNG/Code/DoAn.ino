#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Khởi tạo LCD với địa chỉ I2C 0x27 và kích thước 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

//cau hinh game
unsigned int highScore = 0; // Lưu điểm số cao nhất
int dem = 0;
bool gameStarted = false; // Thêm biến để theo dõi trạng thái bắt đầu game
#define SPRITE_RUN1 1
#define SPRITE_RUN2 2
#define SPRITE_JUMP 3
#define SPRITE_JUMP_UPPER '.'         // Use the '.' character for the head
#define SPRITE_JUMP_LOWER 4
#define SPRITE_TERRAIN_EMPTY ' '      // User the ' ' character
#define SPRITE_TERRAIN_SOLID 5
#define SPRITE_TERRAIN_SOLID_RIGHT 6
#define SPRITE_TERRAIN_SOLID_LEFT 7

#define HERO_HORIZONTAL_POSITION 1    // Horizontal position of hero on screen

#define TERRAIN_WIDTH 16
#define TERRAIN_EMPTY 0
#define TERRAIN_LOWER_BLOCK 1
#define TERRAIN_UPPER_BLOCK 2

#define HERO_POSITION_OFF 0          // Hero is invisible
#define HERO_POSITION_RUN_LOWER_1 1  // Hero is running on lower row (pose 1)
#define HERO_POSITION_RUN_LOWER_2 2  //                              (pose 2)

#define HERO_POSITION_JUMP_1 3       // Starting a jump
#define HERO_POSITION_JUMP_2 4       // Half-way up
#define HERO_POSITION_JUMP_3 5       // Jump is on upper row
#define HERO_POSITION_JUMP_4 6       // Jump is on upper row
#define HERO_POSITION_JUMP_5 7       // Jump is on upper row
#define HERO_POSITION_JUMP_6 8       // Jump is on upper row
#define HERO_POSITION_JUMP_7 9       // Half-way down
#define HERO_POSITION_JUMP_8 10      // About to land

#define HERO_POSITION_RUN_UPPER_1 11 // Hero is running on upper row (pose 1)
#define HERO_POSITION_RUN_UPPER_2 12 //                              (pose 2)

//LiquidCrystal lcd(11, 9, 6, 5, 4, 3);
static char terrainUpper[TERRAIN_WIDTH + 1];
static char terrainLower[TERRAIN_WIDTH + 1];
static bool buttonPushed = false;

static byte heroPos = HERO_POSITION_RUN_LOWER_1;
static byte newTerrainType = TERRAIN_EMPTY;
static byte newTerrainDuration = 1;
static bool playing = false;
static bool blink = false;
static unsigned int distance = 0;

void initializeGraphics() {
  static byte graphics[] = {
    // Run position 1
    B01100,
    B01100,
    B00000,
    B01110,
    B11100,
    B01100,
    B11010,
    B10011,
    // Run position 2
    B01100,
    B01100,
    B00000,
    B01100,
    B01100,
    B01100,
    B01100,
    B01110,
    // Jump
    B01100,
    B01100,
    B00000,
    B11110,
    B01101,
    B11111,
    B10000,
    B00000,
    // Jump lower
    B11110,
    B01101,
    B11111,
    B10000,
    B00000,
    B00000,
    B00000,
    B00000,
    // Ground
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    // Ground right
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    // Ground left
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
  };
  int i;
  // Skip using character 0, this allows lcd.print() to be used to
  // quickly draw multiple characters
  for (i = 0; i < 7; ++i) {
    lcd.createChar(i + 1, &graphics[i * 8]);
  }
  for (i = 0; i < TERRAIN_WIDTH; ++i) {
    terrainUpper[i] = SPRITE_TERRAIN_EMPTY;
    terrainLower[i] = SPRITE_TERRAIN_EMPTY;
  }
}

// Slide the terrain to the left in half-character increments
//
void advanceTerrain(char* terrain, byte newTerrain) {
  for (int i = 0; i < TERRAIN_WIDTH; ++i) {
    char current = terrain[i];
    char next = (i == TERRAIN_WIDTH - 1) ? newTerrain : terrain[i + 1];
    switch (current) {
      case SPRITE_TERRAIN_EMPTY:
        terrain[i] = (next == SPRITE_TERRAIN_SOLID) ? SPRITE_TERRAIN_SOLID_RIGHT : SPRITE_TERRAIN_EMPTY;
        break;
      case SPRITE_TERRAIN_SOLID:
        terrain[i] = (next == SPRITE_TERRAIN_EMPTY) ? SPRITE_TERRAIN_SOLID_LEFT : SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_RIGHT:
        terrain[i] = SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_LEFT:
        terrain[i] = SPRITE_TERRAIN_EMPTY;
        break;
    }
  }
}
bool drawHero(byte position, char* terrainUpper, char* terrainLower, unsigned int score, bool displayHighScore = false) {
  bool collide = false;
  char upperSave = terrainUpper[HERO_HORIZONTAL_POSITION];
  char lowerSave = terrainLower[HERO_HORIZONTAL_POSITION];
  byte upper, lower;

  // Xác định hình ảnh nhân vật theo vị trí
  switch (position) {
    case HERO_POSITION_OFF:
      upper = lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_LOWER_1:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN1;
      break;
    case HERO_POSITION_RUN_LOWER_2:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN2;
      break;
    case HERO_POSITION_JUMP_1:
    case HERO_POSITION_JUMP_8:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_JUMP;
      break;
    case HERO_POSITION_JUMP_2:
    case HERO_POSITION_JUMP_7:
      upper = SPRITE_JUMP_UPPER;
      lower = SPRITE_JUMP_LOWER;
      break;
    case HERO_POSITION_JUMP_3:
    case HERO_POSITION_JUMP_4:
    case HERO_POSITION_JUMP_5:
    case HERO_POSITION_JUMP_6:
      upper = SPRITE_JUMP;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_UPPER_1:
      upper = SPRITE_RUN1;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_UPPER_2:
      upper = SPRITE_RUN2;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
  }

  // Kiểm tra va chạm
  if (upper != ' ') {
    terrainUpper[HERO_HORIZONTAL_POSITION] = upper;
    collide = (upperSave == SPRITE_TERRAIN_SOLID) ? true : collide;
  }
  if (lower != ' ') {
    terrainLower[HERO_HORIZONTAL_POSITION] = lower;
    collide = (lowerSave == SPRITE_TERRAIN_SOLID) ? true : collide;
  }

  // Tính số chữ số của điểm số
  byte digits = (score > 9999) ? 5 : (score > 999) ? 4 : (score > 99) ? 3 : (score > 9) ? 2 : 1;

  // Vẽ cảnh vật trên màn hình
  terrainUpper[TERRAIN_WIDTH] = '\0';
  terrainLower[TERRAIN_WIDTH] = '\0';

  if (!playing) {
    // Chỉ hiển thị điểm cao nhất và không vẽ lại địa hình
    lcd.setCursor(16 - digits, 1);  // Di chuyển điểm số xuống dòng dưới
    lcd.print(score);
  } else {
    lcd.setCursor(0, 0);
    lcd.print(terrainUpper);
    lcd.setCursor(0, 1);
    lcd.print(terrainLower);

    // Hiển thị điểm hiện tại ở góc trên cùng bên phải
    lcd.setCursor(16 - digits, 0);
    lcd.print(score);
  }

  // Chỉ hiển thị điểm cao nhất nếu `displayHighScore` là true
  if (displayHighScore && !playing) {
    lcd.setCursor(0, 1);
    lcd.print("HighScore: ");
    lcd.print(highScore);
  }

  // Khôi phục giá trị ban đầu của `terrainUpper` và `terrainLower`
  terrainUpper[HERO_HORIZONTAL_POSITION] = upperSave;
  terrainLower[HERO_HORIZONTAL_POSITION] = lowerSave;

  return collide;
}
// Cấu hình cho các nút
const int nutTraiPin = 13;
const int nutGiuaPin = 12; // Nút giữa cho menu
const int nutPhaiPin = 11;

// Cấu hình cho các đèn LED
const int ledPins[] = {3, 5, 6};   // Các chân kết nối đèn LED
const int ledButtonPin = 2;        // Chân kết nối nút nhấn

// Biến trạng thái cũ của nút
bool trangThaiNutTraiCu = LOW;
bool trangThaiNutGiuaCu = LOW;
bool trangThaiNutPhaiCu = LOW;

// Biến trạng thái mới của nút
bool trangThaiNutTraiHienTai ;
bool trangThaiNutGiuaHienTai ;
bool trangThaiNutPhaiHienTai ;

// Thời gian chờ để loại bỏ tiếng lắc
const long doTreDebounce = 200;
unsigned long thoiGianBamNut = 0;

// Biến cho menu
int trangThaiMenu = 0;
bool trongMenu = false;
bool trongMenuConLED = false;
bool ledActive = false; // Biến để kiểm soát trạng thái LED
int trangThaiMenuConLED = 0;

// Biến cho cấu hình "XIN CHAO"
String message = "XIN CHAO        ";
int messageLength;
int startIndex = 0;
int scrollCount = 0;
const int maxScrolls = 4;

// Biến cho nhấp nháy
unsigned long previousMillisBlink = 0;
const long blinkInterval = 200;
int cycleCountBlink = 0;
bool isDisplayed = true;

// Biến cho cuộn và thời gian
unsigned long previousMillisScroll = 0;
const int scrollInterval = 200;
unsigned long lastScrollTime = 0;
const unsigned long scrollIntervalCycles = 300;
int scrollState = 1;
int positionCounter = 0;
int cycleCountScroll = 0;
const int maxCycles = 3;
const int screenWidth = 16;

// Biến điều khiển LED
int ledMode = -1;                  // Biến lưu chế độ LED hiện tại
unsigned long previousMillis = 0;   // Để lưu thời gian trước đó cho các hiệu ứng LED

bool inGame = false;
int configNumber = 1;

void setup() {
  pinMode(nutTraiPin, INPUT);
  pinMode(nutGiuaPin, INPUT);
  pinMode(nutPhaiPin, INPUT);

  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  initializeGraphics();
  lcd.init();
  lcd.backlight();
  messageLength = message.length();
  startIndex = messageLength;
}

void loop() {
  unsigned long thoiGianHienTai = millis();

  // Đọc trạng thái nút
  trangThaiNutTraiHienTai = digitalRead(nutTraiPin);
  trangThaiNutGiuaHienTai = digitalRead(nutGiuaPin);
  trangThaiNutPhaiHienTai = digitalRead(nutPhaiPin);

  // Xử lý nút giữa để vào menu hoặc thoát menu
  if (!inGame && !gameStarted && trangThaiNutGiuaHienTai == LOW && trangThaiNutGiuaCu == HIGH && thoiGianHienTai - thoiGianBamNut >= doTreDebounce) {
    thoiGianBamNut = thoiGianHienTai;
    if (!trongMenu && !trongMenuConLED) {
      trongMenu = true;
      trangThaiMenu = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Menu:");
      hienThiMenu();
    } else if (trongMenu && trangThaiMenu == 1) { // Vào menu con LED control
      trongMenu = false;

      trongMenuConLED = true;
      ledActive = false;  // Đảm bảo đèn không hoạt động khi vào menu LED control
      trangThaiMenuConLED = 0;
      hienThiMenuConLED();
    } else if (trongMenuConLED) { // Thoát menu con LED control
      if (trangThaiMenuConLED == 3) { // Nếu chọn "Exit" trong menu con LED
        trongMenuConLED = false;
        trongMenu = true;  // Quay trở lại menu chính
        highScore = 0;
        distance = 0;
        trangThaiMenu = 1; // Đặt lại vị trí ở mục "LED control" trong menu chính
        hienThiMenu();
      } else
      {
        if (ledActive) {  // Nếu đèn đang hoạt động, dừng và quay lại menu con
          ledActive = false;
          turnOffLEDs();
          hienThiMenuConLED();
        } else {  // Nếu đèn không hoạt động, bắt đầu chế độ LED được chọn
          ledMode = trangThaiMenuConLED;
          ledActive = true;
        }
      }
    } else if (trongMenu && trangThaiMenu == 2) { // Nếu chọn "Exit" trong menu chính
      trongMenu = false;
      lcd.clear();            // Xóa màn hình hoàn toàn để không còn chữ "Exit" ở hàng 2
      resetVariables();       // Quay lại cấu hình "XIN CHAO"
    } else if (trongMenu && trangThaiMenu == 0 && inGame == false) {
      trongMenu = false;
      trongMenuConLED = false;
      lcd.clear();
      inGame = true;
    }

  }


  // Xử lý nút phải để di chuyển qua các mục trong menu
  if (trongMenu && trangThaiNutPhaiHienTai == LOW && trangThaiNutPhaiCu == HIGH && thoiGianHienTai - thoiGianBamNut >= doTreDebounce) {
    thoiGianBamNut = thoiGianHienTai;
    if (trangThaiMenu < 2) { // Nếu chưa ở mục cuối cùng ("Exit")
      trangThaiMenu++;
      hienThiMenu();
    }
  }

  //xu ly nut phai de bat dau game
  if (!gameStarted && !trongMenu && inGame == true && trangThaiNutPhaiHienTai == LOW && trangThaiNutPhaiCu == HIGH && thoiGianHienTai - thoiGianBamNut >= doTreDebounce) {
    thoiGianBamNut = thoiGianHienTai;
    if (dem == 0) {
      gameStarted = true; // Đặt biến gameStarted thành true
      playing = false;
      buttonPushed = true;
      dem = 1;
    }
    //buttonPushed = false;
    //dem = 1;
  }
  // Xử lý nút trái để di chuyển ngược lại trong menu
  if (trongMenu && trangThaiNutTraiHienTai == LOW && trangThaiNutTraiCu == HIGH && thoiGianHienTai - thoiGianBamNut >= doTreDebounce) {
    thoiGianBamNut = thoiGianHienTai;
    if (trangThaiMenu > 0) { // Nếu chưa ở mục đầu tiên ("Mini game")
      trangThaiMenu--;
      hienThiMenu();
    }
  }
  //xu ly nut trai de thoat game
  if (gameStarted) {
    if (trangThaiNutGiuaHienTai == LOW && trangThaiNutGiuaCu == HIGH && thoiGianHienTai - thoiGianBamNut >= doTreDebounce) {
      thoiGianBamNut = thoiGianHienTai;
      buttonPushed = true; // Thực hiện lệnh nhảy
    }

  }

  if (trangThaiNutTraiHienTai == LOW && trangThaiNutTraiCu == HIGH && thoiGianHienTai - thoiGianBamNut >= doTreDebounce) {

    if (inGame && gameStarted == false) {
      thoiGianBamNut = thoiGianHienTai;
      inGame = false;
      //playing = false;
      gameStarted = false; // Đặt lại gameStarted để quay về menu
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("back menu");
      delay(1000);
      trongMenu = true;
      hienThiMenu(); // Hiển thị menu
    }
  }


  // Xử lý menu con LED control
  if (trongMenuConLED && !ledActive) {
    if (trangThaiNutPhaiHienTai == LOW && trangThaiNutPhaiCu == HIGH && thoiGianHienTai - thoiGianBamNut >= doTreDebounce) {
      thoiGianBamNut = thoiGianHienTai;
      if (trangThaiMenuConLED < 3) { // Đã tăng lên 3 để bao gồm "Exit"
        trangThaiMenuConLED++;
        hienThiMenuConLED();
      }
    }
    if (trangThaiNutTraiHienTai == LOW && trangThaiNutTraiCu == HIGH && thoiGianHienTai - thoiGianBamNut >= doTreDebounce) {
      thoiGianBamNut = thoiGianHienTai;
      if (trangThaiMenuConLED > 0) {
        trangThaiMenuConLED--;
        hienThiMenuConLED();
      }
    }
  }


  // Thực hiện hiệu ứng LED khi chọn chế độ và đèn đang hoạt động
  if (ledActive) {
    if (ledMode == 0) {
      flashLEDs();
    } else if (ledMode == 1) {
      fadeLEDs();
    } else if (ledMode == 2) {
      ledByLed();
    }
  }

  // Chạy cấu hình "XIN CHAO" nếu không ở trong menu
  if (!trongMenu && !trongMenuConLED && !ledActive && inGame == false) {
    runXinChaoConfig(thoiGianHienTai);
  }

  // chay mini game
  if (inGame) {
    flappyHuman();
  }

  // Cập nhật trạng thái nút
  trangThaiNutTraiCu = trangThaiNutTraiHienTai;
  trangThaiNutGiuaCu = trangThaiNutGiuaHienTai;
  trangThaiNutPhaiCu = trangThaiNutPhaiHienTai;
}

void flappyHuman() {
  if (!playing) {
    // Luôn hiển thị menu khi ở màn hình chờ
    lcd.setCursor(0, 0);
    lcd.print("L:Exit,R:Start");
    
    drawHero(heroPos, terrainUpper, terrainLower, distance >> 3, true);

    // Bắt đầu lại trò chơi khi nút được nhấn
    if (buttonPushed) {
      initializeGraphics();
      heroPos = HERO_POSITION_RUN_LOWER_1;
      playing = true;
      buttonPushed = false;
      distance = 0;
    }
    return;
  }
  else {
    // Di chuyển địa hình sang trái và tạo địa hình mới
    advanceTerrain(terrainLower, newTerrainType == TERRAIN_LOWER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
    advanceTerrain(terrainUpper, newTerrainType == TERRAIN_UPPER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);

    // Tạo địa hình mới ở bên phải màn hình
    if (--newTerrainDuration == 0) {
      if (newTerrainType == TERRAIN_EMPTY) {
        newTerrainType = (random(3) == 0) ? TERRAIN_UPPER_BLOCK : TERRAIN_LOWER_BLOCK;
        newTerrainDuration = 2 + random(10);
      } else {
        newTerrainType = TERRAIN_EMPTY;
        newTerrainDuration = 10 + random(10);
      }
    }

    // Xử lý nhảy khi nút được nhấn
    if (buttonPushed) {
      if (heroPos <= HERO_POSITION_RUN_LOWER_2) heroPos = HERO_POSITION_JUMP_1;
      buttonPushed = false;
    }

    // Kiểm tra va chạm
    if (drawHero(heroPos, terrainUpper, terrainLower, distance >> 3)) {
      playing = false;
      initializeGraphics();
      dem = 0;
      gameStarted = false;
      //menuDisplayed = false;  // Đặt lại để hiển thị lại dòng chữ khi vào lại game
      // Cập nhật điểm số cao nhất nếu điểm hiện tại lớn hơn
      if ((distance >> 3) > highScore) {
        highScore = distance >> 3;
      }
    } else {
      if (heroPos == HERO_POSITION_RUN_LOWER_2 || heroPos == HERO_POSITION_JUMP_8) {
        heroPos = HERO_POSITION_RUN_LOWER_1;
      } else if ((heroPos >= HERO_POSITION_JUMP_3 && heroPos <= HERO_POSITION_JUMP_5) && terrainLower[HERO_HORIZONTAL_POSITION] != SPRITE_TERRAIN_EMPTY) {
        heroPos = HERO_POSITION_RUN_UPPER_1;
      } else if (heroPos >= HERO_POSITION_RUN_UPPER_1 && terrainLower[HERO_HORIZONTAL_POSITION] == SPRITE_TERRAIN_EMPTY) {
        heroPos = HERO_POSITION_JUMP_5;
      } else if (heroPos == HERO_POSITION_RUN_UPPER_2) {
        heroPos = HERO_POSITION_RUN_UPPER_1;
      } else {
        ++heroPos;
      }
      ++distance;


    }
  }


  delay(25);
}

void resetVariables() {
  scrollCount = 0;
  cycleCountBlink = 0;
  cycleCountScroll = 0;
  startIndex = messageLength;
  isDisplayed = true;
  configNumber = 1;
}

void runXinChaoConfig(unsigned long thoiGianHienTai) {
  switch (configNumber) {
    case 1: // Cấu hình 1: Cuộn chữ
      if (scrollCount < maxScrolls) {
        if (thoiGianHienTai - previousMillisScroll >= scrollInterval) {
          previousMillisScroll = thoiGianHienTai;
          for (int i = 0; i <= 16; i++) {
            lcd.setCursor(i, 0);
            lcd.write(message.charAt((startIndex + i) % messageLength));
          }
          startIndex = (startIndex - 1 + messageLength) % messageLength;
          if (scrollCount == maxScrolls - 1 && startIndex == 7) {
            delay(200);
            lcd.clear();
            scrollCount++;

          } else if (startIndex == messageLength - 1) {
            scrollCount++;
          }
        }
      } else {
        //delay(200);
        configNumber = 2;
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("XIN CHAO");
        //delay(500);
      }
      break;

    case 2: // Cấu hình 2: Nhấp nháy chữ
      if (cycleCountBlink < 3) {
        if (thoiGianHienTai - previousMillisBlink >= blinkInterval) {
          previousMillisBlink = thoiGianHienTai;
          if (isDisplayed) {
            lcd.noDisplay();
          } else {
            lcd.display();
            cycleCountBlink++;
          }
          isDisplayed = !isDisplayed;
        }
      } else {
        delay(300);
        configNumber = 3;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("XIN CHAO");
        delay(500);
      }
      break;

    case 3: // Cấu hình 3: Cuộn chữ sang trái và phải
      if (cycleCountScroll >= maxCycles) {
        delay(300);
        configNumber = 1;
        resetVariables();
        return;
      }
      if (thoiGianHienTai - lastScrollTime >= scrollIntervalCycles) {
        lastScrollTime = thoiGianHienTai;
        if (scrollState == 1) {
          lcd.scrollDisplayRight();
          positionCounter++;
          if (positionCounter >= (screenWidth - 3 - 5)) {
            scrollState = 0;

          }
        } else if (scrollState == 0) {
          lcd.scrollDisplayLeft();
          positionCounter--;
          if (positionCounter <= 0) {
            scrollState = 1;
            cycleCountScroll++;
          }
        }
      }
      break;
  }
}

void hienThiMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Menu:");
  lcd.setCursor(0, 1);
  switch (trangThaiMenu) {
    case 0:
      lcd.print("Mini game");
      break;
    case 1:
      lcd.print("LED control");
      break;
    case 2:
      lcd.print("Exit");
      break;
  }
}

void hienThiMenuConLED() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LED control:");
  lcd.setCursor(0, 1);
  switch (trangThaiMenuConLED) {
    case 0:
      lcd.print("Flashing");
      break;
    case 1:
      lcd.print("Fading");
      break;
    case 2:
      lcd.print("Led by Led");
      break;
    case 3:
      lcd.print("Exit"); // Thêm mục Exit vào menu con LED control
      break;
  }
}


void turnOffLEDs() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], LOW);
  }
}

void flashLEDs() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 500) { // Đổi trạng thái LED mỗi 0.5 giây
    previousMillis = currentMillis;
    for (int i = 0; i < 3; i++) {
      digitalWrite(ledPins[i], !digitalRead(ledPins[i])); // Đổi trạng thái LED
    }
  }
}

void fadeLEDs() {
  static int brightness = 0;         // Độ sáng của LED
  static int fadeAmount = 5;         // Mức thay đổi độ sáng mỗi bước
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 30) { // Thay đổi độ sáng mỗi 30ms
    previousMillis = currentMillis;
    for (int i = 0; i < 3; i++) {
      analogWrite(ledPins[i], brightness); // Thiết lập độ sáng
    }
    brightness += fadeAmount;
    if (brightness <= 0 || brightness >= 255) {
      fadeAmount = -fadeAmount; // Đảo chiều độ sáng khi chạm ngưỡng
    }
  }
}

void ledByLed() {
  static int currentLED = 0;         // Chỉ số LED hiện tại
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 300) { // Đổi LED mỗi 0.3 giây
    previousMillis = currentMillis;
    digitalWrite(ledPins[(currentLED + 2) % 3], LOW); // Tắt LED trước đó
    digitalWrite(ledPins[currentLED], HIGH);          // Bật LED hiện tại
    currentLED = (currentLED + 1) % 3;                // Chuyển sang LED tiếp theo
  }
}

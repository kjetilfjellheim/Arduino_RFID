#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>

// RC522 PINS
#define SS_PIN 53
#define RST_PIN 5

// Keypad pins
#define KEYPAD_1 20
#define KEYPAD_2 19
#define KEYPAD_3 18
#define KEYPAD_4 17
#define KEYPAD_5 16
#define KEYPAD_6 15
#define KEYPAD_7 14

// LEDS
#define OPEN_DOOR_PIN 13
#define KEYPAD_PIN 12
#define CLOSED_DOOR_PIN 11

// Key length
#define KEYLENGTH 6

// RFID Reader
MFRC522 mfrc522(SS_PIN, RST_PIN);

// MIFARE key
MFRC522::MIFARE_Key userKey;

// Keypad definition
const byte ROWS = 4; //four rows
const byte COLS = 3; //four columns
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = { KEYPAD_7, KEYPAD_6, KEYPAD_5, KEYPAD_4 };
byte colPins[COLS] = { KEYPAD_3, KEYPAD_2, KEYPAD_1 };
Keypad keypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// Number of keys entered.
int keyNum = 0;

// General state for the applications.
enum State {
  // Waiting for an rfid card
  WAIT_FOR_RFID,
  // Preparing to enter keycode
  PREPARE_KEYPAD_ENTER,
  // Entering keys
  KEYPAD_ENTER,
  // Authenticating key
  AUTHENTICATING,
  // Open door.
  OPEN_DOOR
};

/**
 * User block key.
 */
#define USER_KEY_BLOCK 3


// Setting current state for the application.
State state = WAIT_FOR_RFID;

void setup() {
  Serial.begin(9600);
  Serial.println("Serial started");
  while (!Serial);
  Serial.println("Serial init");
  SPI.begin();
  Serial.println("SPI begun");
  mfrc522.PCD_Init();
  Serial.println("MFRC522 init finished");
  pinMode(OPEN_DOOR_PIN, OUTPUT);  
  pinMode(KEYPAD_PIN, OUTPUT);  
  pinMode(CLOSED_DOOR_PIN, OUTPUT); 
}

/**
 * Application loop.
 */
void loop() {  
  if (state == WAIT_FOR_RFID) {
    if (checkForRfid()) {
      state = PREPARE_KEYPAD_ENTER;
    }
  } else if (state == PREPARE_KEYPAD_ENTER)  {
    keyNum = 0;
    indicateKeypad();
    state = KEYPAD_ENTER;
  } else if (state == KEYPAD_ENTER)  {
    if (keypadEnter()) {
      state = AUTHENTICATING;
    }
  } else if (state == AUTHENTICATING)  {
    if (authenticate()) {
      state = OPEN_DOOR;  
    } else {
      failureKey();
      state = WAIT_FOR_RFID;
    }    
  } else {
    indicateOpenDoor();
    delay(10000);
    indicateClosedDoor();
    state = WAIT_FOR_RFID;
  }
}
/**
 * Checks to see if keypad has been pressed and stores the key.
 */
bool keypadEnter() {
  char keyPressed = keypad.getKey();
  if (keyPressed) {
    userKey.keyByte[keyNum] = (byte)keyPressed;
    Serial.println(keyPressed);
    keyNum = keyNum + 1;
  }  
  if (keyNum == KEYLENGTH) {
    return true;
  } else {
    return false;
  }
}

/**
 * Authenticates the key.
 */
bool authenticate() {
  bool result = false;
  MFRC522::StatusCode status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (byte)USER_KEY_BLOCK, &userKey, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authenticate failed"));
    Serial.println(mfrc522.GetStatusCodeName(status));
  } else {
    Serial.println(F("User authenticate success"));        
    mfrc522.PCD_StopCrypto1();
    mfrc522.PICC_HaltA();
    result = true;    
  }
  return result;
}

/**
 * Keep door locked 3 seconds.
 */
void failureKey() {
  indicateClosedDoor();
  delay(3000);
}

/**
 * Checks for an rfid card.
 */
bool checkForRfid() {
  if (!mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {      
    indicateClosedDoor();
    return false;
  } else {
    Serial.println(F("RFID present."));
    return true;
  }  
}

/**
 * Indicate that the door is open.
 */
void indicateOpenDoor() {
  digitalWrite(OPEN_DOOR_PIN, HIGH);
  digitalWrite(KEYPAD_PIN, LOW);
  digitalWrite(CLOSED_DOOR_PIN, LOW);
}
/**
 * Indicate that the keypad is ready.
 */
void indicateKeypad() {
  digitalWrite(OPEN_DOOR_PIN, LOW);
  digitalWrite(KEYPAD_PIN, HIGH);
  digitalWrite(CLOSED_DOOR_PIN, LOW);
}
/**
 * Indicate that the door is closed.
 */
void indicateClosedDoor() {
  digitalWrite(OPEN_DOOR_PIN, LOW);
  digitalWrite(KEYPAD_PIN, LOW);
  digitalWrite(CLOSED_DOOR_PIN, HIGH);
}


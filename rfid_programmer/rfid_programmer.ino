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
#define WRITE_SUCCESSFUL 13
#define ENTER_NEW_KEY 12
#define ENTER_CURRENT_KEY 11

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
  // Check if default access
  CHECK_DEFAULT_ACCESS,
  // Preparing to enter user keycode
  PREPARE_ENTER_USER_KEY,
  // Preparing to enter keycode
  ENTER_USER_KEY,
  // Authenticate with user key
  AUTHENTICATE_USER_KEY,
  // Prepare entering keys
  PREPARE_ENTER_KEY,
  // Enter key
  ENTERS_NEW_KEY,
  // Write key
  WRITE_KEY
};

/**
   User block key.
*/
const byte USER_KEY_BLOCK = 3;


// Setting current state for the application.
State state = WAIT_FOR_RFID;

/**
   New key to store.
*/
byte newKey[KEYLENGTH];
byte currentKey[KEYLENGTH];

void setup() {
  Serial.begin(9600);
  Serial.println("Serial started");
  while (!Serial);
  Serial.println("Serial init");
  SPI.begin();
  Serial.println("SPI begun");
  mfrc522.PCD_Init();
  Serial.println("MFRC522 init finished");
  pinMode(WRITE_SUCCESSFUL, OUTPUT);
  pinMode(ENTER_NEW_KEY, OUTPUT);
  pinMode(ENTER_CURRENT_KEY, OUTPUT);
}

/**
   Application loop.
*/
void loop() {
  if (state == WAIT_FOR_RFID) {
    indicateNone();
    if (checkForRfid()) {
      setDefaultUserKey();
      state = CHECK_DEFAULT_ACCESS;
    }
  } else if (state == CHECK_DEFAULT_ACCESS)  {
    if (authenticate()) {
      state = PREPARE_ENTER_KEY;
    } else {
      state = PREPARE_ENTER_USER_KEY;
    }
  } else if (state == PREPARE_ENTER_USER_KEY)  {
    keyNum = 0;
    state = ENTER_USER_KEY;
  } else if (state == ENTER_USER_KEY)  {
    if (enterUserKey()) {
      state = AUTHENTICATE_USER_KEY;
    }
  } else if (state == AUTHENTICATE_USER_KEY)  {
    if (authenticate()) {
      state = PREPARE_ENTER_KEY;
    } else {
      state = WAIT_FOR_RFID;
    }
  } else if (state == PREPARE_ENTER_KEY)  {
    keyNum = 0;
    state = ENTERS_NEW_KEY;
  } else if (state == ENTERS_NEW_KEY)  {
    if (enterNewKey()) {
      state = WRITE_KEY;
    }
  } else {
    writeNewCode();      
    state = WAIT_FOR_RFID;
  }
}

/**
   Set default user key.
*/
void setDefaultUserKey() {
  for (int i = 0; i < KEYLENGTH; i++) {
    currentKey[i] = 0xff;
  }
}

/**
   Enter old user key
*/
bool enterUserKey() {
  indicateEnterOldKey();
  char keyPressed = keypad.getKey();
  if (keyPressed) {
    currentKey[keyNum] = (byte)keyPressed;
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
   Enter new user key
*/
bool enterNewKey() {
  indicateEnterNewKey();
  char keyPressed = keypad.getKey();
  if (keyPressed) {
    newKey[keyNum] = (byte)keyPressed;
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
   Write new code to tag.
*/
bool writeNewCode() {
  Serial.println(F("Reading"));
  byte blockData[18];
  byte blockSize = sizeof(blockData);
  byte blockAddr = 3;
  MFRC522::StatusCode status = mfrc522.MIFARE_Read(blockAddr, blockData, &blockSize);
  if (status == MFRC522::STATUS_OK) {
    Serial.println(F("Writing"));    
    blockData[0] = newKey[0];
    blockData[1] = newKey[1];
    blockData[2] = newKey[2];
    blockData[3] = newKey[3];
    blockData[4] = newKey[4];
    blockData[5] = newKey[5];
    status = mfrc522.MIFARE_Write(blockAddr, blockData, 16);
    if (status == MFRC522::STATUS_OK) {
      indicateWriteSuccess();
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    } else {
      Serial.print(F("Write failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      indicateNone();            
    }
    delay(5000);
  } else {
    Serial.print(F("Read failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    indicateNone();
    delay(5000);
  }
}

/**
   Authenticates the key.
*/
bool authenticate() {
  Serial.println(F("Authenticating"));
  bool result = false;
  userKey.keyByte[0] = currentKey[0];
  userKey.keyByte[1] = currentKey[1];
  userKey.keyByte[2] = currentKey[2];
  userKey.keyByte[3] = currentKey[3];
  userKey.keyByte[4] = currentKey[4];
  userKey.keyByte[5] = currentKey[5];
  checkForRfid();
  MFRC522::StatusCode status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (byte)USER_KEY_BLOCK, &userKey, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authenticate failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  } else {
    Serial.println(F("Authenticate success"));  
    result = true;
  }
  return result;
}

/**
   Checks for an rfid card.
*/
bool checkForRfid() {
  if (!mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
    return false;
  } else {
    Serial.println(F("RFID present."));
    return true;
  }
}

/**
   No indicator
*/
void indicateNone() {
  digitalWrite(ENTER_CURRENT_KEY, LOW);
  digitalWrite(ENTER_NEW_KEY, LOW);
  digitalWrite(WRITE_SUCCESSFUL, LOW);
}

/**
    Indicate that the door is open.
*/
void indicateEnterOldKey() {
  digitalWrite(ENTER_CURRENT_KEY, HIGH);
  digitalWrite(ENTER_NEW_KEY, LOW);
  digitalWrite(WRITE_SUCCESSFUL, LOW);
}

/**
    Indicate that the door is open.
*/
void indicateEnterNewKey() {
  digitalWrite(ENTER_CURRENT_KEY, LOW);
  digitalWrite(ENTER_NEW_KEY, HIGH);
  digitalWrite(WRITE_SUCCESSFUL, LOW);
}
/**
    Indicate that the door is closed.
*/
void indicateWriteSuccess() {
  digitalWrite(ENTER_CURRENT_KEY, LOW);
  digitalWrite(ENTER_NEW_KEY, LOW);
  digitalWrite(WRITE_SUCCESSFUL, HIGH);
}


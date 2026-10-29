const int LATCH_PIN = 14;
const int CLOCK_PIN = 27;
const int DATA_PIN = 12;

// Pin Optocoupler Feedback per Rak: {Rak1, Rak2, Rak3, Rak4, Rak5, Rak6}
const int OPTO_PINS[6] = {26, 25, 33, 4, 16, 17};

// Pin Optocoupler buat deteksi barang jatuh
const int DROP_SENSOR_PIN = 32;

// --- SETTING LOGIKA SENSOR BARANG JATUH ---
// Kalau nganggur HIGH, pas lewat LOW -> set LOW
// Kalau nganggur LOW, pas lewat HIGH -> set HIGH
const int DROP_TRIGGER_STATE = LOW; 

unsigned long motorStartTime = 0;
const unsigned long TIMEOUT_MOTOR = 6000; 
bool isMotorRunning = false;
int activeRak = 0; 

// Flag memori sistem
bool itemDropped = false;
bool oneRotationDone = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);

  for (int i = 0; i < 6; i++) {
    pinMode(OPTO_PINS[i], INPUT_PULLUP); 
  }

  pinMode(DROP_SENSOR_PIN, INPUT_PULLUP);

  stopAllMotors();
  Serial.println("Sistem Siap. Semua relay OFF.");
  Serial.println("Ketik format rak,nomor (contoh: 1,10) buat nyalain.");
}

void loop() {
  if (!isMotorRunning && Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    int commaIndex = input.indexOf(',');
    
    if (commaIndex > 0) {
      int rak = input.substring(0, commaIndex).toInt();
      int nomor = input.substring(commaIndex + 1).toInt();
      
      if (rak > 0 && nomor > 0) {
        vendItem(rak, nomor);
      }
    } else {
      Serial.println("Format salah bre! Ketik pake koma, contoh: 1,10");
    }
  }

  if (isMotorRunning) {
    // 1. Pantau drop sensor. 
    // Gua kasih jeda 300ms di awal motor jalan biar gak kena 'noise' tegangan relay
    if (millis() - motorStartTime > 300) {
      if (digitalRead(DROP_SENSOR_PIN) == DROP_TRIGGER_STATE) {
        itemDropped = true;
      }
    }

    // 2. Pantau sensor putaran rak (kasih blind-time 600ms)
    if (!oneRotationDone && (millis() - motorStartTime > 600)) {
      int optoState = digitalRead(OPTO_PINS[activeRak - 1]);
      if (optoState == HIGH) { // Asumsi kelar 1 putaran = HIGH
        oneRotationDone = true; 
        Serial.println("INFO: 1 Putaran beres, ngecek status barang...");
      }
    }

    // 3. Logic Pengeksekusi Kapan Harus Berhenti
    if (oneRotationDone) {
      if (itemDropped) {
        // Barang beneran udah jatuh pas 1 putaran
        stopAllMotors();
        Serial.println("Sempurna bre! 1 Putaran pas & barang jatuh. Motor OFF.");
      } else {
        // Kalo belum jatuh, dia nyuekin 1 putaran ini dan TERUS MUTER
        // Sampe sensor drop ngebaca barang lewat
        if (digitalRead(DROP_SENSOR_PIN) == DROP_TRIGGER_STATE) {
          stopAllMotors();
          Serial.println("Mantap! Barang akhirnya jatuh setelah muter ekstra. Motor OFF.");
        }
      }
    }

    // 4. Timeout pengaman (kalo rak kosong melompong)
    if (millis() - motorStartTime >= TIMEOUT_MOTOR) {
      stopAllMotors();
      Serial.println("TIMEOUT: Motor mati! Barang gak jatuh/rak kosong.");
    }
  }
}

void vendItem(int rak, int nomor) {
  if (rak < 1 || rak > 6 || nomor < 1 || nomor > 10) {
    Serial.println("ERROR: Target rak atau nomor di luar batas sistem.");
    return;
  }
   
  uint16_t bitNomor = 1 << (nomor - 1); 
  uint16_t bitRak = 1 << ((rak - 1) + 10);
  uint16_t shiftData = ~(bitRak | bitNomor);

  Serial.printf("Mengeksekusi Rak: %d, Nomor: %d\n", rak, nomor);
  
  activeRak = rak; 
  itemDropped = false;
  oneRotationDone = false;
  
  updateShiftRegister(shiftData);
  motorStartTime = millis();
  isMotorRunning = true;
}

void stopAllMotors() {
  updateShiftRegister(0xFFFF); 
  isMotorRunning = false;
  activeRak = 0;
}

void updateShiftRegister(uint16_t data) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, highByte(data));
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, lowByte(data));
  digitalWrite(LATCH_PIN, HIGH);
}
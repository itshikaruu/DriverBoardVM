const int LATCH_PIN = 14;
const int CLOCK_PIN = 27;
const int DATA_PIN = 12;

// Pin Optocoupler Feedback per Rak: {Rak1, Rak2, Rak3, Rak4, Rak5, Rak6}
// Full aman sentosa tanpa drama
const int OPTO_PINS[6] = {26, 25, 33, 4, 16, 17};

unsigned long motorStartTime = 0;
const unsigned long TIMEOUT_MOTOR = 4000;
bool isMotorRunning = false;
int activeRak = 0; // Buat nyimpen rak mana yang lagi muter

void setup() {
  Serial.begin(115200);
  
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);

  // Setup pin optocoupler jadi INPUT
  for (int i = 0; i < 6; i++) {
    pinMode(OPTO_PINS[i], INPUT_PULLUP); 
  }

  stopAllMotors();
  Serial.println("Sistem Siap. Semua relay OFF.");
}

void loop() {
  if (!isMotorRunning && Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    // Cari posisi tanda koma
    int commaIndex = input.indexOf(',');
    
    // Kalo ada komanya, pecah jadi dua angka
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

  // Logic pantau motor yg lagi nyala bre
  if (isMotorRunning) {
    // Jedanya gua naikin jadi 600ms biar motor sempet ninggalin titik awal
    if (millis() - motorStartTime > 600) {
      // Baca status optocoupler rak yang lagi jalan (index array mulai dari 0)
      int optoState = digitalRead(OPTO_PINS[activeRak - 1]);

      // Logika diubah jadi HIGH bre
      if (optoState == HIGH) {
        stopAllMotors();
        Serial.println("Mantap bre, 1 putaran kelar (Sensor HIGH). Motor OFF!");
      }
    }

    // Timeout darurat kalo barang nyangkut atau sensor optocoupler rusak
    if (millis() - motorStartTime >= TIMEOUT_MOTOR) {
      stopAllMotors();
      Serial.println("Timeout: Motor berhenti karena nyangkut atau batas waktu habis.");
    }
  }
}

void vendItem(int rak, int nomor) {
  if (rak < 1 || rak > 6 || nomor < 1 || nomor > 10) {
    Serial.println("ERROR: Target rak atau nomor di luar batas sistem.");
    return;
  }
   
  /* 
   * Pemetaan Bit 74HC595 BARU:
   * Bit 0-9   (10 bit) -> Motor Positive (Nomor 1-10)
   * Bit 10-15 (6 bit)  -> Motor Negative (Rak 1-6)
   */
  uint16_t bitNomor = 1 << (nomor - 1); 
  uint16_t bitRak = 1 << ((rak - 1) + 10);
  
  // Di-invert (~) karena relay Active LOW
  uint16_t shiftData = ~(bitRak | bitNomor);

  Serial.printf("Mengeksekusi Rak: %d, Nomor: %d\n", rak, nomor);
  
  activeRak = rak; // Simpen rak mana yang lagi dipake buat patokan baca sensor
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
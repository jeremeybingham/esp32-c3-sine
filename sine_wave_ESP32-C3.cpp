#include <math.h>

// PWM settings
const int pwmPin = 2;
const int pwmFreq = 80000;      // 80 kHz PWM frequency
const int pwmResolution = 8;    // 8-bit (0-255) resolution

// Generator parameters
float currentFreq = 440.0;      // Hz
float amplitude = 1.0;          // 0.0 to 1.0
bool generatorEnabled = true;
String waveform = "sine";       // sine, square, triangle, sawtooth

// Timing
unsigned long lastTime = 0;
float phase = 0;

// Guitar note frequencies
struct Note {
  String name;
  float freq;
};

Note guitarNotes[] = {
  {"E2", 82.4},    // Low E (6th string)
  {"A2", 110.0},   // A (5th string) 
  {"D3", 146.8},   // D (4th string)
  {"G3", 196.0},   // G (3rd string)
  {"B3", 246.9},   // B (2nd string)
  {"E4", 329.6},   // High E (1st string)
  {"A4", 440.0},   // A440 reference
  {"C5", 523.3},   // High C
  {"TEST", 1000.0} // 1kHz test tone
};

void setup() {
  Serial.begin(115200);
  delay(2000);  // Give serial time to initialize
  
  Serial.println("\n\n=== ESP32-C3 Serial Wave Generator ===");
  Serial.println("Initializing...");
  
  // Setup PWM (ESP32-C3 compatible)
  if (ledcAttach(pwmPin, pwmFreq, pwmResolution)) {
    Serial.println("PWM initialized successfully");
  } else {
    Serial.println("PWM initialization failed!");
  }
  
  Serial.println("\n=== CONTROLS ===");
  printHelp();
  printStatus();
}

void loop() {
  // Handle serial commands
  handleSerialCommands();
  
  // Generate waveform
  if (generatorEnabled) {
    generateWaveform();
  } else {
    ledcWrite(pwmPin, 128); // Output DC level when stopped (8-bit midpoint)
  }
  
  delayMicroseconds(25); // ~40kHz sample rate
}

void generateWaveform() {
  unsigned long currentTime = micros();
  float deltaTime = (currentTime - lastTime) / 1000000.0;
  lastTime = currentTime;
  
  phase += 2 * PI * currentFreq * deltaTime;
  if (phase >= 2 * PI) phase -= 2 * PI;
  
  float waveValue = 0;
  
  if (waveform == "sine") {
    waveValue = sin(phase);
  } else if (waveform == "square") {
    waveValue = (sin(phase) > 0) ? 1 : -1;
  } else if (waveform == "triangle") {
    waveValue = (2.0 / PI) * asin(sin(phase));
  } else if (waveform == "sawtooth") {
    waveValue = (phase / PI) - 1;
  }
  
  int pwmValue = (int)(128 + 127 * amplitude * waveValue);
  pwmValue = constrain(pwmValue, 0, 255);
  ledcWrite(pwmPin, pwmValue);
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "help" || command == "h") {
      printHelp();
    }
    else if (command == "status" || command == "s") {
      printStatus();
    }
    else if (command == "on" || command == "start") {
      generatorEnabled = true;
      Serial.println("Generator ON");
      printStatus();
    }
    else if (command == "off" || command == "stop") {
      generatorEnabled = false;
      Serial.println("Generator OFF");
      printStatus();
    }
    else if (command.startsWith("freq ") || command.startsWith("f ")) {
      float newFreq = command.substring(command.indexOf(' ') + 1).toFloat();
      if (newFreq >= 20 && newFreq <= 5000) {
        currentFreq = newFreq;
        Serial.println("Frequency set to " + String(currentFreq) + " Hz");
        printStatus();
      } else {
        Serial.println("Frequency must be between 20 and 5000 Hz");
      }
    }
    else if (command.startsWith("amp ") || command.startsWith("a ")) {
      float newAmp = command.substring(command.indexOf(' ') + 1).toFloat();
      if (newAmp >= 0 && newAmp <= 100) {
        amplitude = newAmp / 100.0;
        Serial.println("Amplitude set to " + String(int(amplitude * 100)) + "%");
        printStatus();
      } else {
        Serial.println("Amplitude must be between 0 and 100%");
      }
    }
    else if (command.startsWith("wave ") || command.startsWith("w ")) {
      String newWave = command.substring(command.indexOf(' ') + 1);
      if (newWave == "sine" || newWave == "square" || newWave == "triangle" || newWave == "sawtooth") {
        waveform = newWave;
        Serial.println("Waveform set to " + waveform);
        printStatus();
      } else {
        Serial.println("Valid waveforms: sine, square, triangle, sawtooth");
      }
    }
    else if (command == "notes" || command == "n") {
      printNotes();
    }
    else if (command.startsWith("note ")) {
      String noteName = command.substring(5);
      noteName.toLowerCase();
      setNoteByName(noteName);
    }
    else if (command == "sweep") {
      sweepTest();
    }
    else if (command.startsWith("tune ")) {
      float delta = command.substring(5).toFloat();
      currentFreq += delta;
      if (currentFreq < 20) currentFreq = 20;
      if (currentFreq > 5000) currentFreq = 5000;
      Serial.println("Frequency adjusted to " + String(currentFreq) + " Hz");
      printStatus();
    }
    else {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}

void printHelp() {
  Serial.println("\n=== AVAILABLE COMMANDS ===");
  Serial.println("help, h           - Show this help");
  Serial.println("status, s         - Show current settings");
  Serial.println("on, start         - Enable generator");
  Serial.println("off, stop         - Disable generator");
  Serial.println("freq <Hz>, f <Hz> - Set frequency (20-5000 Hz)");
  Serial.println("amp <0-100>, a <0-100> - Set amplitude (0-100%)");
  Serial.println("wave <type>, w <type>  - Set waveform (sine/square/triangle/sawtooth)");
  Serial.println("notes, n          - List guitar note presets");
  Serial.println("note <n>          - Set frequency to guitar note");
  Serial.println("tune <±Hz>        - Fine-tune frequency by amount");
  Serial.println("sweep             - Frequency sweep test");
  Serial.println("\nExamples:");
  Serial.println("  freq 440        - Set to A440");
  Serial.println("  amp 75          - Set amplitude to 75%");
  Serial.println("  wave square     - Switch to square wave");
  Serial.println("  note e2         - Set to low E string");
  Serial.println("  tune 0.5        - Increase frequency by 0.5 Hz");
  Serial.println("  tune -1         - Decrease frequency by 1 Hz");
}

void printStatus() {
  Serial.println("\n=== CURRENT STATUS ===");
  Serial.println("Generator: " + String(generatorEnabled ? "ON" : "OFF"));
  Serial.println("Waveform:  " + waveform);
  Serial.println("Frequency: " + String(currentFreq) + " Hz");
  Serial.println("Amplitude: " + String(int(amplitude * 100)) + "%");
  Serial.println("PWM Pin:   GPIO" + String(pwmPin));
  Serial.println("========================");
}

void printNotes() {
  Serial.println("\n=== GUITAR NOTE PRESETS ===");
  for (int i = 0; i < sizeof(guitarNotes) / sizeof(guitarNotes[0]); i++) {
    Serial.println(guitarNotes[i].name + " - " + String(guitarNotes[i].freq) + " Hz");
  }
  Serial.println("\nUsage: note <n> (e.g., 'note e2' or 'note a4')");
}

void setNoteByName(String noteName) {
  for (int i = 0; i < sizeof(guitarNotes) / sizeof(guitarNotes[0]); i++) {
    String name = guitarNotes[i].name;
    name.toLowerCase();
    if (name == noteName) {
      currentFreq = guitarNotes[i].freq;
      Serial.println("Set to " + guitarNotes[i].name + " (" + String(currentFreq) + " Hz)");
      printStatus();
      return;
    }
  }
  Serial.println("Note not found. Type 'notes' to see available notes.");
}

void sweepTest() {
  Serial.println("\n=== FREQUENCY SWEEP TEST ===");
  Serial.println("Sweeping from 100Hz to 1000Hz over 10 seconds...");
  Serial.println("Generator will resume normal operation after sweep.");
  
  bool wasEnabled = generatorEnabled;
  generatorEnabled = true;
  
  unsigned long startTime = millis();
  float startFreq = 100.0;
  float endFreq = 1000.0;
  
  while (millis() - startTime < 10000) { // 10 second sweep
    float progress = (millis() - startTime) / 10000.0;
    currentFreq = startFreq + (endFreq - startFreq) * progress;
    
    // Print progress every second
    if ((millis() - startTime) % 1000 < 50) {
      Serial.println("Sweep: " + String(int(currentFreq)) + " Hz");
    }
    
    generateWaveform();
    delayMicroseconds(25);
  }
  
  generatorEnabled = wasEnabled;
  currentFreq = 440.0; // Reset to A440
  Serial.println("Sweep complete. Reset to 440 Hz.");
  printStatus();
}
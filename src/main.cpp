/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/rfclown
   ________________________________________ */
   
#include "setting.h"
#include "config.h"

void IRAM_ATTR handleButton() {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime > debounceDelay) {
    ChangeRequested = true;
    lastPressTime = currentTime;
  }
}

void IRAM_ATTR handleButton1() {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime > debounceDelay) {
    ChangeRequested1 = true;
    lastPressTime = currentTime;
  }
}

void IRAM_ATTR handleButton2() {
  unsigned long currentTime = millis();
  if (currentTime - lastPressTime > debounceDelay) {
    if (current == DEACTIVE_MODE) current = ACTIVE_MODE;
    else                          current = DEACTIVE_MODE;
    lastPressTime = currentTime;
  }
}

void configure_Radio(RF24 &radio, const byte *channels, size_t size) {
  configureNrf(radio);
  radio.printPrettyDetails();
  for (size_t i = 0; i < size; i++) {
    radio.setChannel(channels[i]);
    radio.startConstCarrier(RF24_PA_MAX, channels[i]);
  }
}

void initialize_MultiMode() {
  if (RadioA.begin()) {
    configure_Radio(RadioA, channelGroup_1, sizeof(channelGroup_1));
  }
  if (RadioB.begin()) {
    configure_Radio(RadioB, channelGroup_2, sizeof(channelGroup_2));
  }
  if (RadioC.begin()) {
    configure_Radio(RadioC, channelGroup_3, sizeof(channelGroup_3));
  }
}

void initialize_Radios() {
  if (current == ACTIVE_MODE) {
    initialize_MultiMode();
  } else if (current == DEACTIVE_MODE) {
    RadioA.powerDown();
    RadioB.powerDown();
    RadioC.powerDown();
    delay(100);
  }
}

static const char* kMenuLabels[] = {
  "WiFi", "Video TX", "RC", "BLE", "Bluetooth", "USB", "Zigbee", "NRF24"
};
static const int kMenuCount = sizeof(kMenuLabels)/sizeof(kMenuLabels[0]);

static int menuIndexFromMode(OperationMode m) {
  switch (m) {
    case WiFi_MODULE:         return 0;
    case VIDEO_TX_MODULE:     return 1;
    case RC_MODULE:           return 2;
    case BLE_MODULE:          return 3;
    case Bluetooth_MODULE:    return 4;
    case USB_WIRELESS_MODULE: return 5;
    case ZIGBEE_MODULE:       return 6;
    case NRF24_MODULE:        return 7;
    default: return 0;
  }
}
static OperationMode modeFromMenuIndex(int idx) {
  switch (idx) {
    case 0: return WiFi_MODULE;
    case 1: return VIDEO_TX_MODULE;
    case 2: return RC_MODULE;
    case 3: return BLE_MODULE;
    case 4: return Bluetooth_MODULE;
    case 5: return USB_WIRELESS_MODULE;
    case 6: return ZIGBEE_MODULE;
    case 7: return NRF24_MODULE;
    default: return WiFi_MODULE;
  }
}

void spectrum() {
  const uint8_t HDR_H = 10;
  const int TOP = HDR_H + 12;
  const int BOT = SCREEN_H - 2;
  const int H   = BOT - TOP;
  const int STRIDE = 3;
  const int BINS   = SCREEN_W / STRIDE;
  const int BAR_W  = 1;
  static uint8_t h[BINS];
  static int8_t  v[BINS];
  static bool init = false;
  if (!init) {
    for (int i = 0; i < BINS; ++i) {
      h[i] = random(0, H + 1);
      v[i] = random(-2, 3);
    }
    init = true;
  }
  for (int i = 0; i < BINS; ++i) {
    int8_t a = (int8_t)random(-1, 2);
    v[i] += a;
    if (v[i] > 3) v[i] = 3;
    if (v[i] < -3) v[i] = -3;
    int16_t nh = (int16_t)h[i] + v[i];
    if (nh < 0) {
      nh = 0;
      v[i] = -(v[i] * 3) / 4;
      if (v[i] == 0) v[i] = 1;
    } else if (nh > H) {
      nh = H;
      v[i] = -(v[i] * 3) / 4;
      if (v[i] == 0) v[i] = -1;
    }
    h[i] = (uint8_t)nh;
  }
  for (int i = 0; i < BINS; ++i) {
    int x  = i * STRIDE;
    int bh = h[i];
    if (bh <= 0) continue;
    int y  = BOT - bh;
    u8g2.drawVLine(x, y, bh);
  }
  u8g2.sendBuffer();
}

// ---------------- New UI: two-panel RF-NPI screen ----------------
// Panel izquierdo (branding + rana). Panel derecho (herramienta + toggle).
// Coordenadas basadas en el diseno de lopaka (posicion de fuente por baseline).

static const uint8_t* FONT_TOOL   = u8g2_font_haxrcorp4089_tr; // nombre de herramienta / estado
static const uint8_t* FONT_BRAND  = u8g2_font_4x6_tr;          // "RF-NPI" / version

static const int RB_X      = 60;   // x del panel derecho
static const int RB_W      = 66;   // ancho del panel derecho
static const int RB_INX0   = 62;   // interior izq (para clip de texto)
static const int RB_INX1   = 125;  // interior der
static const int TOOL_CX   = 93;   // centro horizontal del texto en panel derecho
static const int TOOL_Y    = 16;   // baseline nombre de herramienta
static const int STATUS_Y  = 55;   // baseline texto de estado

static void drawCenteredX(int centerX, int y, const char* s) {
  int w = u8g2.getStrWidth(s);
  u8g2.drawStr(centerX - (w / 2), y, s);
}

// Dibuja todo el marco fijo (paneles, rana, flechas, toggle, estado y branding).
// El nombre de la herramienta se dibuja aparte para poder animarlo.
static void drawPanels(bool active) {
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFontPosBaseline();
  u8g2.setDrawColor(1);

  // Panel derecho (marco redondeado)
  u8g2.drawRFrame(RB_X, 2, RB_W, 60, 5);

  // Rana
  u8g2.drawXBMP(8, 11, 46, 42, image_npi_bits);

  // Panel izquierdo (marco + barras superior/inferior)
  u8g2.drawLine(2, 60, 2, 3);
  u8g2.drawLine(59, 8, 2, 8);
  u8g2.drawLine(59, 55, 2, 55);
  u8g2.drawLine(62, 2, 2, 2);
  u8g2.drawBox(2, 2, 58, 7);
  u8g2.drawLine(63, 61, 2, 61);
  u8g2.drawXBMP(60, 3, 2, 2, image_paint_11_bits);
  u8g2.drawBox(2, 55, 59, 6);
  u8g2.drawXBMP(61, 60, 1, 1, image_paint_13_bits);

  // Toggle segun estado
  u8g2.drawXBMP(80, 23, 27, 17, active ? image_toggle_active_bits : image_toggle_deactive_bits);

  // Estado (texto bajo el toggle)
  u8g2.setFont(FONT_TOOL);
  drawCenteredX(TOOL_CX, STATUS_Y, active ? "ACTIVE" : "DEACTIVE");

  // Ojo de la rana solo en DEACTIVE
  if (!active) {
    u8g2.drawXBMP(43, 20, 2, 2, image_paint_11_bits);
  }

  // Flechas de navegacion
  u8g2.drawXBMP(63, 28, 4, 7, image_ButtonLeft_bits);
  u8g2.drawXBMP(119, 28, 4, 7, image_ButtonRight_bits);

  // Branding (XOR sobre las barras del panel izquierdo)
  u8g2.setDrawColor(2);
  u8g2.setFont(FONT_BRAND);
  u8g2.drawStr(20, 8, "RF-NPI");
  u8g2.drawStr(18, 61, "V 1.0.0");
  u8g2.setDrawColor(1);
}

static void drawScreenNPI(const char* tool, bool active) {
  u8g2.clearBuffer();
  drawPanels(active);
  u8g2.setFont(FONT_TOOL);
  drawCenteredX(TOOL_CX, TOOL_Y, tool);
  u8g2.sendBuffer();
}

static void renderStaticMenu(int focusIndex) {
  drawScreenNPI(kMenuLabels[focusIndex], current == ACTIVE_MODE);
}

// Anima el cambio de herramienta deslizando su nombre dentro del panel derecho.
// dir = +1 -> siguiente (entra desde la derecha), dir = -1 -> anterior.
static void animateToMenu(int fromIdx, int toIdx) {
  const char* fromTool = kMenuLabels[fromIdx];
  const char* toTool   = kMenuLabels[toIdx];
  bool active = (current == ACTIVE_MODE);

  int diff = toIdx - fromIdx;
  int dir  = (diff > 0) ? 1 : -1;
  if (diff ==  (kMenuCount - 1)) dir = -1; // wrap 0 -> last
  if (diff == -(kMenuCount - 1)) dir =  1; // wrap last -> 0

  const int SPAN  = 46;   // recorrido del deslizamiento
  const int STEPS = 12;
  const uint8_t DT = 12;
  for (int s = 0; s <= STEPS; s++) {
    float t = (float)s / (float)STEPS;
    float e = (t < 0.5f) ? 4.0f*t*t*t : 1.0f - powf(-2.0f*t + 2.0f, 3)/2.0f;
    int shift = (int)(e * SPAN + 0.5f);
    u8g2.clearBuffer();
    drawPanels(active);
    u8g2.setFont(FONT_TOOL);
    u8g2.setClipWindow(RB_INX0, 3, RB_INX1, 25);
    drawCenteredX(TOOL_CX - dir * shift,          TOOL_Y, fromTool);
    drawCenteredX(TOOL_CX + dir * (SPAN - shift), TOOL_Y, toTool);
    u8g2.setMaxClipWindow();
    u8g2.sendBuffer();
    delay(DT);
  }
  drawScreenNPI(toTool, active);
}

// Cambia el estado active/deactive con una breve confirmacion visual.
static void animateToggleKnobForFocus(int focusIdx, bool fromActive, bool toActive){
  (void)fromActive;
  drawScreenNPI(kMenuLabels[focusIdx], toActive);
}

void update_OLED() {
  renderStaticMenu(menuIndexFromMode(current_Mode));
}

void menuPrev() {
  int fromIdx = menuIndexFromMode(current_Mode);
  int toIdx   = (fromIdx == 0) ? (kMenuCount - 1) : (fromIdx - 1);
  current_Mode = modeFromMenuIndex(toIdx);
  animateToMenu(fromIdx, toIdx);
}
void menuNext() {
  int fromIdx = menuIndexFromMode(current_Mode);
  int toIdx   = (fromIdx == (kMenuCount - 1)) ? 0 : (fromIdx + 1);
  current_Mode = modeFromMenuIndex(toIdx);
  animateToMenu(fromIdx, toIdx);
}
void menuToggleActive() {
  int focus = menuIndexFromMode(current_Mode);
  bool wasActive = (current == ACTIVE_MODE);
  current = wasActive ? DEACTIVE_MODE : ACTIVE_MODE;
  animateToggleKnobForFocus(focus, wasActive, !wasActive);
}

void checkMode() {
  if (ChangeRequested) {
    ChangeRequested = false;
    current_Mode = static_cast<OperationMode>((current_Mode == 0) ? 7 : (current_Mode - 1));
  } else if (ChangeRequested1) {
    ChangeRequested1 = false;
    current_Mode = static_cast<OperationMode>((current_Mode + 1) % 8);
  }
}

void setup() {
  Serial.begin(115200);
  initialize_MultiMode();
  Wire.begin();
  Wire.setClock(400000);
  u8g2.begin();
  u8g2.setBusClock(400000);
  u8g2.setFont(FONT_SMALL);
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  esp_bt_controller_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_wifi_disconnect();
  pinMode(PIN_BTN_L,  INPUT_PULLUP);
  pinMode(PIN_BTN_R, INPUT_PULLUP);
  pinMode(PIN_BTN_S, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_L),  handleButton,  FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_R), handleButton1, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_S), handleButton2, FALLING);
  initialize_Radios();
  conf();
  update_OLED();
}

void loop() {
  // Actualizar el LED según el estado actual
  digitalWrite(LED_PIN, (current == ACTIVE_MODE) ? HIGH : LOW);

  checkMode();
  static Operation     lastActivity = current;
  static OperationMode lastFocus    = current_Mode;
  if (current_Mode != lastFocus) {
    int fromIdx = menuIndexFromMode(lastFocus);
    int toIdx   = menuIndexFromMode(current_Mode);
    animateToMenu(fromIdx, toIdx);
    lastFocus = current_Mode;
    return;
  }
  if (current != lastActivity) {
    initialize_Radios();
    int  focus     = menuIndexFromMode(current_Mode);
    bool wasActive = (lastActivity == ACTIVE_MODE);
    bool nowActive = (current      == ACTIVE_MODE);
    animateToggleKnobForFocus(focus, wasActive, nowActive);
    lastActivity = current;
    return;
  }
    if (current_Mode == BLE_MODULE) {
      int randomIndex = random(0, sizeof(ble_channels) / sizeof(ble_channels[0]));
      byte channel = ble_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == Bluetooth_MODULE) {
      int randomIndex = random(0, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0]));
      byte channel = bluetooth_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == WiFi_MODULE) {
      int randomIndex = random(0, sizeof(WiFi_channels) / sizeof(WiFi_channels[0]));
      byte channel = WiFi_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == USB_WIRELESS_MODULE) {
      int randomIndex = random(0, sizeof(usbWireless_channels) / sizeof(usbWireless_channels[0]));
      byte channel = usbWireless_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == VIDEO_TX_MODULE) {
      int randomIndex = random(0, sizeof(videoTransmitter_channels) / sizeof(videoTransmitter_channels[0]));
      byte channel = videoTransmitter_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == RC_MODULE) {
      int randomIndex = random(0, sizeof(rc_channels) / sizeof(rc_channels[0]));
      byte channel = rc_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == ZIGBEE_MODULE) {
      int randomIndex = random(0, sizeof(zigbee_channels) / sizeof(zigbee_channels[0]));
      byte channel = zigbee_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    } else if (current_Mode == NRF24_MODULE) {
      int randomIndex = random(0, sizeof(nrf24_channels) / sizeof(nrf24_channels[0]));
      byte channel = nrf24_channels[randomIndex];
      RadioA.setChannel(channel);
      RadioB.setChannel(channel);
      RadioC.setChannel(channel);
    }
}
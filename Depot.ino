// ------------------------------------------------------------
// Lagerverwaltungssystem main
// Janick Wäspi 
// 12.04.2023
// Metrohm AG
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000);

  SetupDepot();
  InitializeBoxes();
  delay(1000);
}

void loop() {
  UpdateBoxes();
  delay(2000);
}

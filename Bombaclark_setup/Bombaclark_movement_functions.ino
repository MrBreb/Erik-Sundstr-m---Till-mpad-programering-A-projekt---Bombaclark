/**
* @file Bombaclark_movement_functions.ino
* @brief Rörelsefunktioner, animationer och seriell styrning för manuella tester och kalibreringar.
* @author Erik Sundström
* @date 2026-05-21
* * Beskrivning:
* Denna kod tar innehåller all Bombaclarks rörelselogik. Koden ser till att 
* servona rör sig i rätt ordning för att bilda gångstilar och innehåller 
* Utkommenterad kod som kollar efter och utför kommandon som skickas in via 
* serial monitorn för manuella tester och servo-kalibreringar.
* * Hårdvarukrav & I/O:
* - Adafruit PCA9685 via I2C, pin 6 (SDA) och pin 7 (SCL).
* - 8 st SG90 servon som ansluts till kanalerna 0 till 7 på PCA9685-drivern
*/

int hip_pos[4] = {neutral_pos, neutral_pos, neutral_pos, neutral_pos}; ///< En array som lagrar vilken position varje hip servo har - 0:fl 1:fr 2:bl 3:br
int leg_pos[4] = {neutral_pos, neutral_pos, neutral_pos, neutral_pos}; ///< En array som lagrar vilken position varje ben servo har - 0:fl 1:fr 2:bl 3:br

// ========================================
// Definerar namn åt alla servon.
// ========================================
const int FL_LEG = 4, FL_HIP = 3;
const int FR_LEG = 5, FR_HIP = 2;
const int BL_LEG = 6, BL_HIP = 1;
const int BR_LEG = 7, BR_HIP = 0;

/**
* @brief Styr ett specifikt servo till en önskad vinkel (pos variabeln).
* @param which_servo Vilekn servo som ska styras.
* @param pos Önskad vinkel mellan 0 och 180 grader.
* @note Eftersom att FL_HIP, BL_HIP, FR_LEG och BL_LEG är inverterade har jag satt att pos = 180 - pos 
* när which servo är en av dessa. Då ifall pos är 40 så blir det istället 140, vise-versa.
*/ 
void move_servo(int which_servo, int pos){
  if (which_servo == FR_LEG) pos = 190 - pos; ///< FR_LEG är på grund av hur fästet fungerar inverterad och lite felmonterat i jämförelse med resten av benen, 190 - which_servo fixar både inverteringen och monterings felet
  if (which_servo == BR_LEG && pos != 0) pos = pos - 15; ///< FR_LEG är på grund av hur fästet fungerar lite felmonterat i jämförelse med resten av benen, pos + 10 fixar monterings felet
  if (which_servo == FL_HIP || which_servo == BL_HIP || which_servo == BL_LEG) pos = 180 - pos;
  int pulse = map(pos, 0, 180, SERVOMIN, SERVOMAX); 
  pwm.setPWM(which_servo, 0, pulse);
}

/**
* @brief Översätter en textsträng för ett ben till ett numeriskt index (0-3).
* @param limb Sträng som representerar vilket ben man ska kolla ID på ("fl", "fr", "bl", "br").
* @return Benets ID-index (0 för fl, 1 för fr, 2 för bl, 3 för br).
*/ 
int get_limb_id(String limb){
  if      (limb == "fl") return 0;
  else if (limb == "fr") return 1;
  else if (limb == "bl") return 2;
  else if (limb == "br") return 3;
}

/**
* @brief Funktion som underlättar att röra ett ben, används även i funktionen "single_step".
* @param Which_limb Sträng för vilket ben som ska röras.
* @param which_pos Önskad vinkelposition för det valda benet.
*/ 
void move_one_leg(String Which_limb, int which_pos){
  int id = get_limb_id(Which_limb);
  leg_pos[id] = which_pos;
  if      (Which_limb == "fl") move_servo(FL_LEG, which_pos);  
  else if (Which_limb == "fr") move_servo(FR_LEG, which_pos);
  else if (Which_limb == "bl") move_servo(BL_LEG, which_pos); 
  else if (Which_limb == "br") move_servo(BR_LEG, which_pos);
}

/**
* @brief Funktion som underlättar att ta ett steg. Vrid ben uppåt, flytta fram höft, vrid ben neråt.
* @param Which_limb Sträng för vilket ben som ska ta steget.
* @param which_pos Önskad höftvinkel för steget.
* @note Innehåller matematisk invertering för de servon som är fysiskt spegelvända på robotkroppen.
*/ 
void single_step(String Which_limb, int which_pos){
  int id = get_limb_id(Which_limb);
  if (hip_pos[id] != which_pos){
    move_one_leg(Which_limb, up_pos);
    delay(base_delay);

    if      (Which_limb == "fl") {move_servo(FL_HIP, which_pos);  hip_pos[0] = which_pos;} 
    else if (Which_limb == "fr") {move_servo(FR_HIP, which_pos);  hip_pos[1] = which_pos;}
    else if (Which_limb == "bl") {move_servo(BL_HIP, which_pos);  hip_pos[2] = which_pos;} 
    else if (Which_limb == "br") {move_servo(BR_HIP, which_pos);  hip_pos[3] = which_pos;}
    delay(base_delay);

    move_one_leg(Which_limb, down_pos);
    delay(base_delay);
  }
}

/**
* @brief Återställer alla servopositioner till den valnliga stående positionen.
* @note Funktionen flyttar först till pos 110 för att visualisera återställninen.
*/ 
void idle_reset(){ 
  single_step("fl", 110);
  single_step("fl", neutral_pos);
  single_step("fr", 110);
  single_step("fr", neutral_pos);
  single_step("bl", 110);
  single_step("bl", neutral_pos);
  single_step("br", 110);
  single_step("br", neutral_pos);
}

/**
* @brief Återställer alla servopositioner till den stående positionen.
* @note Denna funktionen gör inget om alla servos redan är i den stående positionen, körs när kommando kön är slut.
*/ 
void idle(){
  single_step("fl", neutral_pos);
  single_step("fr", neutral_pos);
  single_step("bl", neutral_pos);
  single_step("br", neutral_pos);
}

/**
* @brief Flyttar alla ben framåt ett i taget och sedan puttar alla ben bakåt samtidigt.
* @note Bombaclark måste gå så här eftersom den är fyrbent och bara har två leder per ben, 
* när Bombaclark går som ett vanlig djur blir den väldigt ostabil vilket gör att den ramlar 
* hela tiden och inte kommer någon vart.
*/ 
void walk_forward(){
  /* Flyttar alla ben framåt, ett ben i taget */
  single_step("bl", front_pos);
  single_step("fl", front_pos);
  single_step("br", front_pos);
  single_step("fr", front_pos);

  /* Trycker ifrån med alla ben samtidigt, flyttar bara höfterna */
  move_servo(FL_HIP, back_pos); hip_pos[0] = back_pos; 
  move_servo(FR_HIP, back_pos);       hip_pos[1] = back_pos; 
  move_servo(BL_HIP, back_pos); hip_pos[2] = back_pos; 
  move_servo(BR_HIP, back_pos);       hip_pos[3] = back_pos;
  
  delay(big_delay);
}

/**
* @brief Flyttar alla ben bakåt ett i taget och sedan drar alla ben framåt samtidigt.
* @note Bombaclark måste gå så här eftersom den är fyrbent och bara har två leder per ben, 
* när Bombaclark går som ett vanlig djur blir den väldigt ostabil vilket gör att den ramlar 
* hela tiden och inte kommer någon vart.
*/ 
void walk_backward(){
  /* Flyttar alla ben bakåt, ett ben i taget */
  single_step("bl", back_pos);
  single_step("fl", back_pos);
  single_step("br", back_pos);
  single_step("fr", back_pos);

  /* Drar fram alla ben samtidigt, flyttar bara höfterna */
  move_servo(FL_HIP, front_pos); hip_pos[0] = front_pos; 
  move_servo(FR_HIP, front_pos);       hip_pos[1] = front_pos;
  move_servo(BL_HIP, front_pos); hip_pos[2] = front_pos;
  move_servo(BR_HIP, front_pos);       hip_pos[3] = front_pos;

  delay(big_delay);
}

/**
* @brief Flyttar alla ben till höger ett i taget och sedan drar alla ben åt vänster samtidigt.
*/ 
void walk_right(){
  /* Flyttar alla ben till höger, ett ben i taget */
  single_step("bl", back_in_pos);            
  single_step("fl", front_in_pos);   
  single_step("br", back_out_pos);    
  single_step("fr", front_out_pos);       

  /* Flyttar alla ben till vänster samtidigt, flyttar bara höfterna */
  move_servo(BL_HIP, back_out_pos);  hip_pos[2] = back_out_pos; 
  move_servo(FL_HIP, front_out_pos); hip_pos[0] = front_out_pos; 
  move_servo(BR_HIP, back_in_pos);   hip_pos[3] = back_in_pos;
  move_servo(FR_HIP, front_in_pos);  hip_pos[1] = front_in_pos;

  delay(big_delay);
}

/**
* @brief Flyttar alla ben till vänster ett i taget och sedan drar alla ben åt höger samtidigt.
* @note Alla 180 - pos är en temporär korrigering
*/ 
void walk_left(){
  /* Flyttar alla ben till vänster, ett ben i taget */
  single_step("bl", back_out_pos);
  single_step("fl", front_out_pos);
  single_step("br", back_in_pos);
  single_step("fr", front_in_pos);

  /* Flyttar alla ben till höger samtidigt, flyttar bara höfterna */
  move_servo(BL_HIP, back_in_pos);    hip_pos[2] = back_in_pos;
  move_servo(FL_HIP, front_in_pos);   hip_pos[0] = front_in_pos;
  move_servo(BR_HIP, back_out_pos);   hip_pos[3] = back_out_pos;
  move_servo(FR_HIP, front_out_pos);  hip_pos[1] = front_out_pos;

  delay(big_delay);
}

/**
* @brief Får Bombaclark att vrida upp ena sidan av sin kropp och vinka med sitt högra framben.
* @note Positionerar först benen så att roboten lutar snett bakåt för stabilitet, sedan
* vinkar den 5 gånger i en förinställd hastighet.
*/ 
void wave(){
  // Sätter kroppen i rätt position
  idle();
  single_step("fl", front_pos);
  single_step("br", front_pos);
  move_servo(BL_LEG, 140);
  hip_pos[2] = back_pos;
  single_step("fr", 40);

  // Vinkar
  for(int i = 0; i < 5; i++){
    move_servo(FR_LEG, 110);
    delay(medium_delay);
    move_servo(FR_LEG, 160);
    delay(medium_delay);
  }
  delay(base_delay);
}

/**
* @brief Gör att Bombaclark nickar (säger ja) genom att vrida frambenen uppåt och neråt tre gånger.
*/ 
void yes(){
  for(int i = 0; i < 3; i++){
    move_servo(FL_LEG, 60);
    move_servo(FR_LEG, 60);
    delay(medium_delay);
    move_servo(FL_LEG, down_pos);
    move_servo(FR_LEG, down_pos);
    delay(medium_delay);
  }
  delay(base_delay);
}

/**
* @brief Får Bombaclark att skaka på huvudet (säga nej) genom att vrida de främre höfterna fram och tillbaka tre gånger.
*/ 
void no(){
  for(int i = 0; i < 3; i++){
    move_servo(FL_HIP, 140);
    move_servo(FR_HIP, 180 - 140);
    delay(medium_delay);
    move_servo(FL_HIP, 40);
    move_servo(FR_HIP, 180 - 40);
    delay(medium_delay);
  }
  move_servo(FL_HIP, neutral_pos);
  move_servo(FR_HIP, neutral_pos);
  delay(base_delay);
}

String input_string = ""; 
int temp_int = 90;

/**
* @brief Tar emot textinput från Serial Monitor och konverterar det till ett heltal (integer).
* @return Det inmatade heltalet från Serial Monitor.
* @note Denna funktion används enbart för manuell kalibrering av enskilda servon. Loopen 
* väntar aktivt tills data finns tillgängligt för att inte läsa tomma strängar.
*/ 
int get_integer() {
  while (Serial.available() == 0) { 
    delay(10); 
  }             
  String input = Serial.readStringUntil('\n');
  return input.toInt();
}

/**
* @brief Testnings- och kalibrerings loop som lyssnar efter manuella kommandon via Serial Monitor.
* @note Används enbart för manuella tester och servo-kalibrering, bör vara utkommenterad när den 
* inte används då programmet kraschar om det finns två void loop() funktioner samtidigt.
*/ 
//void loop() {
//  while (Serial.available() > 0){
//    input_string = Serial.readStringUntil('\n'); 
//    input_string.trim();
//
//    if      (input_string == "f") walk_forward();
//    else if (input_string == "b") walk_backward();
//    else if (input_string == "r") walk_right();
//    else if (input_string == "l") walk_left();
//    else if (input_string == "i") idle_reset();
//    else if (input_string == "w") wave();
//    else if (input_string == "y") yes();
//    else if (input_string == "n") no();
//
//    else if (input_string == "1") {temp_int = get_integer(); move_servo(0, temp_int);}
//    else if (input_string == "2") {temp_int = get_integer(); move_servo(1, temp_int);}
//    else if (input_string == "3") {temp_int = get_integer(); move_servo(2, temp_int);}
//    else if (input_string == "4") {temp_int = get_integer(); move_servo(3, temp_int);}
//    else if (input_string == "5") {temp_int = get_integer(); move_servo(4, temp_int);}
//    else if (input_string == "6") {temp_int = get_integer(); move_servo(5, temp_int);}
//    else if (input_string == "7") {temp_int = get_integer(); move_servo(6, temp_int);}
//    else if (input_string == "8") {temp_int = get_integer(); move_servo(7, temp_int);}
//  }
//  idle();
//  delay(10); 
//}
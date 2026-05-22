# ==========================================
# Filnamn: Final_transcribe--AI--bombaclark.py
# Beskrivning: Transcriberar vad man säger, skickar det till en lokal AI och skickar sedan det svaret till Bombaclark via wifi
# Upphovsman: Erik Sundström
# Datum: 2026-05-08
# ==========================================

import sounddevice as sd
import numpy as np
import requests
import keyboard # Så att jag kan använda enter
from faster_whisper import WhisperModel

model = WhisperModel("small", device="cpu", compute_type="int8")
LM_STUDIO_URL = "http://localhost:1234/v1/chat/completions"

ESP32_IP = "172.20.10.2" # Byt ut mot IP-adressen du ser i Arduinos Serial Monitor (192.168.1.2 när jag är hemma) (192.168.192.160 när jag är i skolan)

def send_to_robot(commands_string):                             #Denna koden är AI genererad                            
    # Delar upp kommandon till en lista                         #Denna koden är AI genererad                                
    commands = [c.strip() for c in commands_string.split(",")]  #Denna koden är AI genererad                                                        
                                                                #Denna koden är AI genererad
    for cmd in commands:                                        #Denna koden är AI genererad                    
        if not cmd: continue                                    #Denna koden är AI genererad                        
        try:                                                    #Denna koden är AI genererad        
            print(f"Skickar till robot: {cmd}")                 #Denna koden är AI genererad                                        
            # Vi skickar kommandot via en HTTP GET request      #Denna koden är AI genererad                                                    
            url = f"http://{ESP32_IP}/command"                  #Denna koden är AI genererad                                        
            requests.get(url, params={"cmd": cmd}, timeout=5)   #Denna koden är AI genererad                                                        
        except Exception as e:                                  #Denna koden är AI genererad                        
            print(f"Kunde inte nå roboten: {e}")                #Denna koden är AI genererad                                            


# ask_ai är en funktion som skickar det man säger, system prompten och vilka inställningar som ska användas till LM-studios
def ask_ai(text):
    system_prompt = """You are the Bombaclark Logic Engine. Your SOLE purpose is to convert human speech into a comma-separated list of robot commands. Sometimes the human speach will be nonsensical, in that case you should do your best to find clues for what they are trying to communicate. If there are no clues then say: "I understand. Let's begin." AND NOTHING ELSE.

COMMAND LIBRARY (Use ONLY these):
walk left, walk right, walk forward, walk backward, wave, idle, yes and no.

STRICT RULES:
1. Output ONLY the commands.
2. DO NOT include "Input:", "Output:", or any conversational filler.
3. Use a comma to separate multiple commands.
4. You are not allowed to guess what the human is trying to say, you may use clues but you CAN NOT guess.
5. You are ONLY allowed to use commands from the command library. If none of the commands in the command library fits what the human is saying, then say: "I understand. Let's begin." and nothing else.

EXAMPLES:
User: Gå frammåt några gånger och sen gå höger.
Assistant: "walk forward, walk forward, walk forward, walk right".

User: Säg hej.
Assistant: "wave".

User: Är två plus två fyra.
Assistant: "yes"."""

    payload = {
        "model": "google/gemma-3-4b",
        "messages":[
        {"role": "system", "content": system_prompt},
        {"role": "user", "content": text} ], # Här sätts det man säger in
        "temperature": 0
    }
    try: # Denna koden tills "return f"Fel: {e}"" är inspirerad av lånad kod
        response = requests.post(LM_STUDIO_URL, json=payload, timeout=30)  # "LM_STUDIO_URL" är vart AI modellen hostas på datorn. "payload" är insställningarna t.ex vilken modell som ska laddas in. "timeout=30" betyder att den kommer försöka i 30 sekunder sedan köra except.
        return response.json()['choices'][0]['message']['content'].strip() # ".json()" gör att texten från servern blir till ett python objekt. "['choices'][0]" väljer det första svaret AI:n ger ifall den hadde gett flera svar. "['message']['content']" tar den texten AI:n skriver och ".strip()" tar bort onödiga karaktärer som mellandrum.
    except Exception as error_message: # error_message är felmedelandet ifall try skulle misslyckas
        return f"Fel: {error_message}"

# Record_and_process är en funktion som, efter att enter plvit tryckt lyssnar på det man säger och med hjälp av AI genererad kod gör om det till text.
def record_and_process():
    print("\n--- KLAR ---") # Skriver ut "--- KLAR ---" när alla andra processer är klara och programmet är redo för att köras
    print("Tryck ENTER för att starta inspelning")

    while True: # Väntar på att man ska säga något
        keyboard.wait('enter')
        print("LYSSNAR... (Tryck ENTER igen för att avsluta inspelning)")

        audio_data = []                                                                     # Denna kod är AI genererad                    
                                                                                            # Denna kod är AI genererad
        # Sparar allt ljud i en lista                                                       # Denna kod är AI genererad                                    
        def callback(indata, frames, time, status):                                         # Denna kod är AI genererad                                                
            audio_data.append(indata.copy())                                                # Denna kod är AI genererad                                            
                                                                                            # Denna kod är AI genererad
        with sd.InputStream(samplerate=16000, channels=1, callback=callback):               # Denna kod är AI genererad                                                                            
            keyboard.wait('enter') # väntar till enter trycks ner igen                      # Denna kod är AI genererad                                                                    
                                                                                            # Denna kod är AI genererad
        print("BEARBETAR...")                                                               # Denna kod är AI genererad                            
                                                                                            # Denna kod är AI genererad
        # Detta kombinerar allt ljud som spelats in                                         # Denna kod är AI genererad                                                
        full_audio = np.concatenate(audio_data, axis=0).flatten().astype(np.float32)        # Denna kod är AI genererad                                                                                    
                                                                                            # Denna kod är AI genererad
        # Detta transciberar det kombinerade ljudet på en gång                              # Denna kod är AI genererad                                                            
        segments, _ = model.transcribe(full_audio, beam_size=5, language="sv")              # Denna kod är AI genererad                                                                            
        user_text = " ".join([s.text.strip() for s in segments])                            # Denna kod är AI genererad                                                                

        if len(user_text) > 1: # Detta ser till så att den faktiskt hörde något, det betyder att det användaren säger måste vara längre än en bokstav.
            print(f"Du sa: {user_text}")
            result = ask_ai(user_text)
            print(f"AI Svar: {result}")
            if result == "I understand. Let's begin.": # "I understand. Let's begin." är vad AI:n borde säga när den inte hittade något giltikt kommando.
                print("Det fanns inget giltigt kommando i det du sa, var snäll och säg ett giltigt kommando (Ex: Gå frammåt).")
            else:
                send_to_robot(result) # Send to robot är en AI:genererad funktion som skickar "result" (vad AI:n skickade som svar) till roboten över wifi
        else:
            print("Hörde inget, testa igen.")

        print("\nKlar. Tryck ENTER för att ge nytt kommando.")

if __name__ == "__main__":      # Detta är AI genrerad kod                
    try:                        # Detta är AI genrerad kod
        record_and_process()    # Detta är AI genrerad kod                    
    except KeyboardInterrupt:   # Detta är AI genrerad kod                    
        print("\nAvslutat.")    # Detta är AI genrerad kod                    

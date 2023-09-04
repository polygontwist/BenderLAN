# BenderLAN
Ein ESP8266-Webserver zur Ansteuerung meiner Bender-Figur (https://www.thingiverse.com/thing:6200580).

Da das [Phythonboard (Bender-PHY)](https://github.com/polygontwist/Bender-PHY) keine Netzwerk-Schnittstelle besitzt, übernimmt der ESP8266 diese Aufgabe.

Auf dem ESP läuft ein Webserver, der Kommandos entgegen nimmt und an das Phythonboard weiterleitet. Dafür wird die serielle Schnittstelle mit 115200 baut benutzt.

So kann man z.B. sich die Zeit ansagen lassen, eine zufällige Kopfanimation starten oder stoppen, das Datum erfragen oder das Phythonscript beenden. Man kann auch die LED auf dem ESP ein- und ausschalten.

![screenshot_1](https://github.com/polygontwist/BenderLAN/blob/main/benderLAN.png)

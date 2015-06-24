# pebble-geocachingpro

_Geocaching Pro_ for Pebble Time watches

Based on the _Get Back To_ app from Peter Summers (https://apps.getpebble.com/applications/550bf37f1e540c856300005e) which is based on the _Get Back_ app from Samuel Rinnetmäki (https://apps.getpebble.com/applications/52cae46a6126ce093f000064). Inspired by the _Find Cache_ app also from Samuel Rinnetmäki (https://apps.getpebble.com/applications/540ca9ef1120350bc20001e0).

Thanks to Samuel and Peter for laying the foundation, building and enhacing the apps! Your apps were the first, that got installed on my Pebble Time! I intended to use them primarly for Geocachung. Unfortunately they did not exactly meet all my expectations and wishes regarding my Geocaching use cases. So I just started looking if I could do anything to take those approaches even further and enhance or rebuild those apps. After a little bit investigation and analysis I decided to start with the _Get Back To_ app from Peter as a foundation.

Main issue: You can add targets (e.g. coordinates of geocaches) only via the settings page, which naturaly requires internet connectivity. Since Geocaching (at least for me) is often about stepping out into the wild wild open, the phone often lags from responsive and capable internet connectivity. Therefore I have my Geocaching app preloaded with map data and geocache information. To increase battery life of the phone I also try to minimize network connectivity to a minimum, often completely disabling WiFi and mobile network capabilities. In this scenario, I'm unfortunately not able to push a geocache location to my Pebble Time, because all of the apps mentioned above are relying on the online settings page or on receiving timeline pins for transmitting the information. That does not work out under weak or even missing mobile connectivity. And for me as a computer scientist it is also a little bit squirky to require online services to push something over from my phone to a localy connected Pebble watch.   

Here is my current plan and backlog of changes and improvements:

Done
- Getting the source codes of the apps, setting the develop and build environment up and starting with a clean compile of the original apps.

Ongoing
- Offline-Setting-Page vorgelagert zu Online-Setting-Page um neue Ziele auch ohne Netzwerkverbindung übertragen zu können. Status: Hierzu habe ich erste Recherchen und Versuche betrieben. Mit Data URI sieht das vielversprechend aus. 

ToDo
- Offline-Setting-Page mit Feld für die schnelle Übernahme eines neuen Ziels. Ziel wird zu oberst in die Liste eingetragen, direkt als Ziel übernommen und die Navigation gestartet.
- Starten der Navigation im Auto-Mode, oben ist Bewegungsrichtung - Tweak und Optimierung der Standardparameter analog aktueller Einstellungen. 
- Hervorhebung des Auswahlcursor im Menü (Zieleliste) fixen.
- Change the color feedback while navigating from nautic scheme to a more intuitive scheme for non-nautics. Green = Getting closer, target ahead. Yellow = Getting closer but target off to the side. Red = Getting away from target, wrong direction.
- Neuer Name, Credits, Referenz auf Urheber. 
- Test mit Android. Test auf Pebble und Pebble Steel. 
- Public release im Pebble App Store. Beschreibungstext. Funktionen. 
- Später: UI überarbeiten. Geocaching App und neuen Kompass als Basis? Uhrzeit und Datum für Logs einblenden, evtl nur wenn Uhr gekickt wird.
- Später: Stromverbrauch optimieren. API Calls optimieren. Hintergrundaktivität optimieren. UI optimieren auf möglichst wenig und kleinflächige E-Paper Updates. 
- Später: Komplett auf Offline-Setting-Page umstellen.

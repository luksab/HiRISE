# Arbeitsschritte für HiRISE
Der gesamte code ist auf [GitHub.com/luksab/HiRISE](https://github.com/luksab/hirise) zu finden
# Planung

### Storyboard
![](images/storyBoard1.jpg)

###Story
Der Protagonist steht von seinem Schreibtisch auf und schaut sich um - es starten Sirenen und er fängt an zum Fenster zu laufen.
Auf dem Weg dahin wird er von mehreren Personen verfolgt und macht etwas Parkour über Tische und andere Hindernisse.
Er bricht durch das Fenster und man sieht das Gebäude von außen, welches sich auf einem Science-Fiction Mars befindet und springt auf ein gegenübergelegenes Haus und läuft weg.

### Technisches
#### Features zu implementieren
- [x] Minimalistische Cinematic Engine
    - [x] Datenformat aus Blender exportieren, in Programm importieren
        - einfach dae/obj
    - [x] protobuf oder json als Dateiformat? - Custom (floats in Datei, space-separated)
    - [x] Kameraposition, Skelettanimationen
        - [x] Kameraposionen speichen
- [x] Rigging für Charaktere
    - [x] muss sehr gut funktion, um realistische Bewegungen zu erlauben
- [ ] Physikalische Simulation der Glassplitter
    - kann auch in Blender vorsimuliert werden, sollte es nicht in echtzeit möglich sein
- [x] Reflexion im Glas
    - [x] Zunächst bei intakter Scheibe durch duplikation der Szene unsetzbar
    - [ ] Bei vielen Glassplittern andere Technik notwendig
        - [x] Environment Map reflektieren
        - [ ] Environment Map in realtime neu erzugen
    - [x] z.B. SS (unvollständig), Ray-tracing (zu teuer), Environment Map
- [x] Dispacement Mapping + PDS Parser
    - [x] Höhendaten und Bilder von Marsoberfläche aus PDS Daten laden
    - [x] Marsoberfläche von HiRISE-daten
- [ ] Sound
    - Etwas Musikunterlage
        - [x] Code
        - [ ] Musik selbst
#### Optionale Features
- [x] Dynamische Subdivision Surface der Marsoberfläche
    - Erhöht sowohl Performance, als auch Qualität
- [ ] Prozedurale Texturen für Mars-oberfläche
    - [x] Farben für Flächen, auf denen die HiRISE Kamera keine Farbinformationen erhalten hat
- [ ] HDR-Effekte, Bewegungsunschärfe, Fokusunschärfe
    - [ ] Qualität erhöhen, wenn noch Zeit und Performace über ist (Priorität absteigend)

## Praktisches
### Charakteranimation
Um die animation des Charakters realistisch zu gestalten wurden Referenzvideos aufgenommen, die dann in Blender als Hintergrund benutzt wurden, um dann mithilfe von inverse kinematics zu animieren.
![](images/blender_animation_setup.jpg)
Die Referenzvideos sind mit der expliziten Erlaubnis von Ben Karcher aufgenommen und verwendet.
![](images/BenKarcherErlaubnis.jpg)

###Marsoberfläche
Da als Umgebung eine Marsoberfläche gewünscht wurde, werden Höhendaten und ein Schwarz-Weiß Bild vom High Resolution Imaging Science Experiment (HiRISE, daher stammt auch der Projektname) benutzt.
Der genaue Datensatz lässt sich unter https://www.uahirise.org/dtm/dtm.php?ID=ESP_048136_1725 finden.

Um aus den Höhendaten und dem Schwarz-Weiß Bild ein 3D-Objekt mit Farben zu erstellen wurde ein Referenzbild in Farbe benutzt, um ein Node-Netzwerk in Blender anzupassen, um nahe an das Referenzbild zu gelangen.
![](images/nodeNetwork.jpg)
In Blender lassen sich in Echtzeit Parameter und Zusammenhänge ausprobieren um schnell zu iterieren und so zu einer Lösung zu kommen. Dieses Node-Netzwerk wurde dann in Shader-Code umgesetzt.
![](images/marsShader.jpg)
### Mars in HiRISE gerendered
![](images/marsDemo.jpg)

## Kamera und UI
Hier ist die Entwicklungsumgebung mit allen UI-Elementen aktiviert:
![](images/Screenshot_UI_demo.jpg)
Zur Entwicklung des Benutzerinterfaces wurde dear imgui benutzt. Dies erlaubt schnell relativ komlexe Bedienelemente einzubinden.
Die einzelnen Elemente lassen sich deaktivieren, um Platz zu sparen.

### Kamerafahrt
Diese wird mithilfe von Splines, welche durch Punkte, durch die die Kamera gehen soll definiert werden.
Hier ist die erste Kamerafahrt. Bei dieser sind die Tangenten noch nicht korrekt gesetzt, weswegen sie etwas abgehackt erscheint.
<figure class="video_container">
  <video controls="true" allowfullscreen="true">
    <source src="./videos/first Cam.mp4" type="video/mp4">
  </video>
</figure>

## PBR & HDRI
Außerhalb des Gebäude wird zur beleuchtung ein HDRI nach dem Vorbild von https://learnopengl.com/PBR/IBL/Specular-IBL benutzt.

## Skeletal animation
Das importieren von Skeletal Animationen hat sich als recht fragil herausgestellt und bei Exporteinstellungen, die von denen abweichen, die in [blenderExport](blenderExport.md) beschrieben werden, geht das Modell fast sicher kaputt.
<figure class="video_container">
  <video controls="true" allowfullscreen="true">
    <source src="./videos/brokenSkelett.mp4" type="video/mp4">
  </video>
</figure>
Es werden die einzelnen Knochen rekursiv von der Hüfte aus importiert, indem die Transformationsmatrizen pro frame immer an die Kinder weitergegeben werden. Dadurch werden, wenn z.B der Arm bewegt wird die Hand und dadurch auch die Hand mitbewegt.

Hier ist der Anfang in Blender Animiert:
<figure class="video_container">
  <video controls="true" allowfullscreen="true">
    <source src="./videos/animationFromScratch.mp4" type="video/mp4">
  </video>
</figure>

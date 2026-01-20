# SEMESTRÁLNÍ PROJEKT - ANALÝZA BĚŽECKÝCH AKTIVIT

Mým projektem je konzolová C++ aplikace, která načítá, analyzuje a zobrazuje běžecké aktivity uložené ve formátu .gpx. Tento formát jsem zvolil z důvodu hodinek Garmin, na kterých si běhání měřím, tudíž jsem naimportoval předešlé tréninky z aplikace Strava do projektu, která tento formát podporuje. Speciální funkcí je i zobrazení běhu na stránce Mapy.cz.

#FUNKCE PROJEKTU
-
V programu jsou dostupné následující funkce:
-> Vypsat seznam aktivit (Program načítá jednotlivé aktivity, které já ručně nahraji do složky "data")
-> Detail aktivity (Detaily jsou den, měsíc, rok kdy jsem běžel, vzdálenost, čas průměrné tempo a tempo za jednotlivé kilometry. Specialitou je i nejrychlejší kilometr ze všech uběhnutých)
-> Tydenni trend (Program vypíše za jednotlivé týdny v roce počet běhů, uběhnutých kilometrů a jejich tempa. Na základě tohoto se může uživatel hodnotit a plánovat své tréninky do budoucna podle toho, co je jeho cílem)
-> Znovu nacist data a seradit (Tato funkce funguje čistě jako synchronizace úložiště pro tréninky.)
-> Export aktivity pro Mapy.cz (Možnost pro uživatele zrekapitulovat si svoji trasu kudy běžel)
-> Konec (Ukončení programu)

#KOSTRA PROJEKTU
-
Projekt sem rozdělil do několika částí:
-> Activity (Obsahuje základní informace o aktivitách a uchovává seznam GPS bodů)
-> GpxParser (Funkce nutná pro správné zobrazení tohoto formátu uložení tréninku. Řeší souřadnice, čas, atd.)
-> Stats (Výpočty vzdálenosti, tempa, splitů a týdenních trendů)
-> GpxExporter (Umožňuje nahrát aktivitu na Mapy.cz a zobrazit uběhnutou trasu)
-> Main (Zobrazuje výstup celého programu, se kterým uživatel interaguje)
________________________
|__data
|--Activity.h / .cpp
|--GpxParser.h / .cpp
|--GpxExporter.h / .cpp
|--Stats.h / .cpp
|--main.cpp
|--README.md
________________________

       




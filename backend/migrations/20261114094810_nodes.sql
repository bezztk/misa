-- Add migration script here
BEGIN;

DO $misa_import$
DECLARE
v_data JSONB := $misa_json$
{
  "domains": [
    {
      "name": "Mathematik",
      "nodes": [
        {
          "name": "Mengenlehre und Grundlagen der Mathematik",
          "children": [
            {
              "name": "Arithmetik und Zahlensysteme"
            },
            {
              "name": "Elementare Mengenlehre"
            },
            {
              "name": "Axiomatische Mengenlehre"
            },
            {
              "name": "Kardinal- und Ordinalzahltheorie"
            },
            {
              "name": "Deskriptive Mengenlehre"
            }
          ]
        },
        {
          "name": "Mathematische Logik und Beweistheorie",
          "children": [
            {
              "name": "Aussagenlogik"
            },
            {
              "name": "Prädikatenlogik"
            },
            {
              "name": "Nichtklassische Logiken"
            },
            {
              "name": "Modelltheorie"
            },
            {
              "name": "Beweistheorie"
            },
            {
              "name": "Berechenbarkeitstheorie"
            },
            {
              "name": "Typentheorie"
            }
          ]
        },
        {
          "name": "Zahlentheorie",
          "children": [
            {
              "name": "Elementare Zahlentheorie"
            },
            {
              "name": "Analytische Zahlentheorie"
            },
            {
              "name": "Algebraische Zahlentheorie"
            },
            {
              "name": "Geometrische Zahlentheorie"
            },
            {
              "name": "Diophantische Analysis"
            },
            {
              "name": "Algorithmische Zahlentheorie"
            },
            {
              "name": "Probabilistische Zahlentheorie"
            }
          ]
        },
        {
          "name": "Algebra und lineare Algebra",
          "children": [
            {
              "name": "Elementare Algebra"
            },
            {
              "name": "Lineare und multilineare Algebra"
            },
            {
              "name": "Gruppentheorie"
            },
            {
              "name": "Ring- und Modultheorie"
            },
            {
              "name": "Körper- und Galoistheorie"
            },
            {
              "name": "Kommutative Algebra"
            },
            {
              "name": "Universelle Algebra"
            },
            {
              "name": "Lie-Theorie"
            },
            {
              "name": "Darstellungstheorie"
            },
            {
              "name": "Kategorientheorie"
            },
            {
              "name": "Homologische Algebra"
            }
          ]
        },
        {
          "name": "Analysis und Differentialgleichungen",
          "children": [
            {
              "name": "Differential- und Integralrechnung"
            },
            {
              "name": "Reelle Analysis"
            },
            {
              "name": "Komplexe Analysis"
            },
            {
              "name": "Maß- und Integrationstheorie"
            },
            {
              "name": "Funktionalanalysis"
            },
            {
              "name": "Operatorentheorie"
            },
            {
              "name": "Harmonische Analysis"
            },
            {
              "name": "Gewöhnliche Differentialgleichungen"
            },
            {
              "name": "Partielle Differentialgleichungen"
            },
            {
              "name": "Integralgleichungen"
            },
            {
              "name": "Dynamische Systeme und Ergodentheorie"
            },
            {
              "name": "Variationsrechnung"
            }
          ]
        },
        {
          "name": "Geometrie",
          "children": [
            {
              "name": "Euklidische Geometrie"
            },
            {
              "name": "Analytische Geometrie"
            },
            {
              "name": "Affine Geometrie"
            },
            {
              "name": "Projektive Geometrie"
            },
            {
              "name": "Nicht-euklidische Geometrie"
            },
            {
              "name": "Metrische Geometrie"
            },
            {
              "name": "Differentialgeometrie"
            },
            {
              "name": "Algebraische Geometrie"
            },
            {
              "name": "Konvexe Geometrie"
            },
            {
              "name": "Diskrete Geometrie"
            }
          ]
        },
        {
          "name": "Topologie",
          "children": [
            {
              "name": "Allgemeine Topologie"
            },
            {
              "name": "Algebraische Topologie"
            },
            {
              "name": "Geometrische Topologie"
            },
            {
              "name": "Differentialtopologie"
            },
            {
              "name": "Knotentheorie"
            }
          ]
        },
        {
          "name": "Diskrete Mathematik",
          "children": [
            {
              "name": "Abzählende Kombinatorik"
            },
            {
              "name": "Algebraische Kombinatorik"
            },
            {
              "name": "Extremale Kombinatorik"
            },
            {
              "name": "Graphentheorie"
            },
            {
              "name": "Matroidtheorie"
            },
            {
              "name": "Kombinatorische Designtheorie"
            },
            {
              "name": "Kodierungstheorie"
            }
          ]
        },
        {
          "name": "Wahrscheinlichkeitstheorie und stochastische Prozesse",
          "children": [
            {
              "name": "Maßtheoretische Wahrscheinlichkeitstheorie"
            },
            {
              "name": "Grenzwerttheorie"
            },
            {
              "name": "Markow-Prozesse"
            },
            {
              "name": "Martingaltheorie"
            },
            {
              "name": "Stochastische Analysis"
            },
            {
              "name": "Stochastische Geometrie"
            },
            {
              "name": "Theorie der Zufallsfelder"
            }
          ]
        },
        {
          "name": "Statistik",
          "children": [
            {
              "name": "Deskriptive Statistik"
            },
            {
              "name": "Inferenzstatistik"
            },
            {
              "name": "Bayessche Statistik"
            },
            {
              "name": "Nichtparametrische Statistik"
            },
            {
              "name": "Multivariate Statistik"
            },
            {
              "name": "Regressionsanalyse"
            },
            {
              "name": "Zeitreihenanalyse"
            },
            {
              "name": "Stichprobentheorie"
            },
            {
              "name": "Versuchsplanung"
            },
            {
              "name": "Räumliche Statistik"
            },
            {
              "name": "Statistische Lerntheorie"
            }
          ]
        },
        {
          "name": "Numerische Mathematik",
          "children": [
            {
              "name": "Numerische lineare Algebra"
            },
            {
              "name": "Numerische Lösung nichtlinearer Gleichungen"
            },
            {
              "name": "Numerische Approximation und Interpolation"
            },
            {
              "name": "Numerische Differentiation und Integration"
            },
            {
              "name": "Numerische Lösung gewöhnlicher Differentialgleichungen"
            },
            {
              "name": "Numerische Lösung partieller Differentialgleichungen"
            },
            {
              "name": "Numerische Optimierung"
            },
            {
              "name": "Monte-Carlo-Methoden"
            }
          ]
        },
        {
          "name": "Optimierung und Operations Research",
          "children": [
            {
              "name": "Lineare Optimierung"
            },
            {
              "name": "Nichtlineare Optimierung"
            },
            {
              "name": "Konvexe Optimierung"
            },
            {
              "name": "Ganzzahlige Optimierung"
            },
            {
              "name": "Kombinatorische Optimierung"
            },
            {
              "name": "Stochastische Optimierung"
            },
            {
              "name": "Mehrkriterielle Optimierung"
            },
            {
              "name": "Dynamische Programmierung"
            },
            {
              "name": "Optimale Steuerung"
            },
            {
              "name": "Spieltheorie"
            },
            {
              "name": "Entscheidungstheorie"
            },
            {
              "name": "Warteschlangentheorie"
            },
            {
              "name": "Netzwerk- und Ablaufplanung"
            }
          ]
        },
        {
          "name": "Mathematische Modellierung und angewandte Mathematik",
          "children": [
            {
              "name": "Mathematische Physik"
            },
            {
              "name": "Mathematische Biologie"
            },
            {
              "name": "Mathematische Ökonomie"
            },
            {
              "name": "Finanz- und Versicherungsmathematik"
            },
            {
              "name": "Technomathematik"
            },
            {
              "name": "Mathematische Systemtheorie"
            },
            {
              "name": "Theorie inverser Probleme"
            }
          ]
        }
      ]
    },
    {
      "name": "Informatik und Informationswissenschaft",
      "nodes": [
        {
          "name": "Theoretische Informatik",
          "children": [
            {
              "name": "Algorithmentheorie"
            },
            {
              "name": "Datenstrukturen"
            },
            {
              "name": "Automaten- und Formalsprachentheorie"
            },
            {
              "name": "Berechenbarkeit und Entscheidbarkeit"
            },
            {
              "name": "Komplexitätstheorie"
            },
            {
              "name": "Formale Spezifikation und Verifikation"
            },
            {
              "name": "Semantik von Programmen"
            },
            {
              "name": "Theorie der Nebenläufigkeit"
            }
          ]
        },
        {
          "name": "Informationswissenschaft",
          "children": [
            {
              "name": "Wissensorganisation und Klassifikation"
            },
            {
              "name": "Informationserschließung und Metadaten"
            },
            {
              "name": "Informationssuche und Informationswiedergewinnung"
            },
            {
              "name": "Informationsverhalten und Informationsnutzung"
            },
            {
              "name": "Informationsarchitektur"
            },
            {
              "name": "Informationsmanagement"
            },
            {
              "name": "Wissensmanagement"
            },
            {
              "name": "Bibliothekswissenschaft"
            },
            {
              "name": "Archivwissenschaft"
            },
            {
              "name": "Bibliometrie und Szientometrie"
            }
          ]
        },
        {
          "name": "Computational Science",
          "children": [
            {
              "name": "Computergestützte Modellbildung"
            },
            {
              "name": "Numerische Simulation"
            },
            {
              "name": "Hochleistungsrechnen"
            },
            {
              "name": "Parallele Simulationsverfahren"
            },
            {
              "name": "Unsicherheitsquantifizierung und Sensitivitätsanalyse"
            }
          ]
        },
        {
          "name": "Data Science",
          "children": [
            {
              "name": "Datengewinnung und Datenaufbereitung"
            },
            {
              "name": "Datenqualität und Datenvalidierung"
            },
            {
              "name": "Explorative Datenanalyse"
            },
            {
              "name": "Statistische Datenmodellierung"
            },
            {
              "name": "Datenbergbau und Musterentdeckung"
            },
            {
              "name": "Prädiktive Analytik"
            },
            {
              "name": "Kausale Datenanalyse"
            }
          ]
        },
        {
          "name": "Programmierung und Programmiersprachen",
          "children": [
            {
              "name": "Programmierparadigmen"
            },
            {
              "name": "Programmiersprachentheorie und Sprachentwurf"
            },
            {
              "name": "Typsysteme"
            },
            {
              "name": "Compilerbau und Übersetzertechnik"
            },
            {
              "name": "Laufzeitsysteme und virtuelle Maschinen"
            },
            {
              "name": "Statische und dynamische Programmanalyse"
            },
            {
              "name": "Nebenläufige und parallele Programmierung"
            },
            {
              "name": "Metaprogrammierung und Programmtransformation"
            }
          ]
        },
        {
          "name": "Softwaretechnik",
          "children": [
            {
              "name": "Anforderungsanalyse und Anforderungsmanagement"
            },
            {
              "name": "Softwarearchitektur und Softwareentwurf"
            },
            {
              "name": "Softwareentwicklungsprozesse und Methoden"
            },
            {
              "name": "Softwareprojektmanagement"
            },
            {
              "name": "Softwarequalität und Qualitätssicherung"
            },
            {
              "name": "Softwaretest und Testautomatisierung"
            },
            {
              "name": "Softwarewartung und Softwareevolution"
            },
            {
              "name": "Versions-, Build- und Release-Management"
            },
            {
              "name": "Kontinuierliche Integration und Bereitstellung"
            }
          ]
        },
        {
          "name": "Betriebssysteme",
          "children": [
            {
              "name": "Betriebssystemarchitektur und Kernelentwicklung"
            },
            {
              "name": "Prozess- und Threadverwaltung"
            },
            {
              "name": "Prozesskommunikation und Synchronisation"
            },
            {
              "name": "Speicherverwaltung"
            },
            {
              "name": "Dateisysteme und Massenspeicherverwaltung"
            },
            {
              "name": "Geräteverwaltung und Ein-/Ausgabesysteme"
            },
            {
              "name": "Virtualisierung und Containerisierung"
            },
            {
              "name": "Echtzeitbetriebssysteme"
            }
          ]
        },
        {
          "name": "Rechnerarchitektur und Computersysteme",
          "children": [
            {
              "name": "Digitale Rechnerorganisation"
            },
            {
              "name": "Prozessor- und Befehlssatzarchitekturen"
            },
            {
              "name": "Speicher- und Cachearchitekturen"
            },
            {
              "name": "Ein-/Ausgabe- und Bussysteme"
            },
            {
              "name": "Parallele und heterogene Rechnerarchitekturen"
            },
            {
              "name": "Eingebettete Systeme"
            },
            {
              "name": "Rekonfigurierbare Rechnersysteme"
            },
            {
              "name": "Hardware-Software-Koentwurf"
            }
          ]
        },
        {
          "name": "Computernetzwerke",
          "children": [
            {
              "name": "Netzwerkarchitekturen und Referenzmodelle"
            },
            {
              "name": "Netzwerkprotokolle und Kommunikationsverfahren"
            },
            {
              "name": "Vermittlungs- und Routingverfahren"
            },
            {
              "name": "Netzwerkübertragungs- und Verbindungstechnologien"
            },
            {
              "name": "Drahtlose und mobile Netzwerke"
            },
            {
              "name": "Internetarchitektur und Internetdienste"
            },
            {
              "name": "Netzwerkmanagement und Netzwerküberwachung"
            },
            {
              "name": "Softwaredefinierte und virtualisierte Netzwerke"
            }
          ]
        },
        {
          "name": "Datenbanken und Informationssysteme",
          "children": [
            {
              "name": "Datenmodellierung und Datenbankentwurf"
            },
            {
              "name": "Relationale Datenbanksysteme"
            },
            {
              "name": "Nichtrelationale Datenbanksysteme"
            },
            {
              "name": "Datenbanksprachen und Anfrageverarbeitung"
            },
            {
              "name": "Transaktions- und Nebenläufigkeitsverwaltung"
            },
            {
              "name": "Verteilte Datenbanksysteme"
            },
            {
              "name": "Datenintegration und Datenföderation"
            },
            {
              "name": "Datenlager und analytische Informationssysteme"
            },
            {
              "name": "Betriebliche Informationssysteme"
            },
            {
              "name": "Datenbankadministration und Datenbankbetrieb"
            }
          ]
        },
        {
          "name": "IT-Sicherheit und Kryptografie",
          "children": [
            {
              "name": "Kryptografie und kryptografische Protokolle"
            },
            {
              "name": "Kryptoanalyse"
            },
            {
              "name": "System- und Hardwaresicherheit"
            },
            {
              "name": "Anwendungssicherheit"
            },
            {
              "name": "Netzwerksicherheit"
            },
            {
              "name": "Identitäts- und Zugriffsmanagement"
            },
            {
              "name": "Schwachstellenanalyse und Sicherheitsprüfung"
            },
            {
              "name": "Schadsoftwareanalyse"
            },
            {
              "name": "Digitale Forensik"
            },
            {
              "name": "Datenschutztechnik und Anonymisierung"
            }
          ]
        },
        {
          "name": "Künstliche Intelligenz",
          "children": [
            {
              "name": "Symbolische künstliche Intelligenz"
            },
            {
              "name": "Wissensrepräsentation und automatisches Schlussfolgern"
            },
            {
              "name": "Maschinelles Lernen"
            },
            {
              "name": "Tiefes Lernen"
            },
            {
              "name": "Bestärkendes Lernen"
            },
            {
              "name": "Generative Modelle"
            },
            {
              "name": "Automatische Planung und Entscheidungsfindung"
            },
            {
              "name": "Natürliche Sprachverarbeitung"
            },
            {
              "name": "Maschinelle Sprachverarbeitung und Spracherkennung"
            },
            {
              "name": "Maschinelles Sehen und Bildverstehen"
            },
            {
              "name": "Evolutionäre Berechnungsverfahren"
            },
            {
              "name": "Intelligente Agenten und Multiagentensysteme"
            }
          ]
        },
        {
          "name": "Computergrafik und Visualisierung",
          "children": [
            {
              "name": "Zweidimensionale Computergrafik"
            },
            {
              "name": "Dreidimensionale Modellierung"
            },
            {
              "name": "Bildsynthese und Rendering"
            },
            {
              "name": "Computeranimation"
            },
            {
              "name": "Echtzeitgrafik"
            },
            {
              "name": "Digitale Bildverarbeitung"
            },
            {
              "name": "Wissenschaftliche Visualisierung"
            },
            {
              "name": "Informationsvisualisierung"
            },
            {
              "name": "Virtuelle und erweiterte Realität"
            }
          ]
        },
        {
          "name": "Mensch-Computer-Interaktion",
          "children": [
            {
              "name": "Benutzeroberflächengestaltung"
            },
            {
              "name": "Interaktionsdesign"
            },
            {
              "name": "Gebrauchstauglichkeit und Nutzererfahrung"
            },
            {
              "name": "Nutzerforschung und menschzentrierte Entwicklung"
            },
            {
              "name": "Barrierefreie und inklusive Gestaltung"
            },
            {
              "name": "Multimodale Interaktion"
            },
            {
              "name": "Immersive Interaktion"
            }
          ]
        },
        {
          "name": "Verteilte Systeme",
          "children": [
            {
              "name": "Verteilte Systemarchitekturen"
            },
            {
              "name": "Kommunikation und Koordination verteilter Systeme"
            },
            {
              "name": "Konsistenz und Replikation"
            },
            {
              "name": "Fehlertoleranz und Verfügbarkeit"
            },
            {
              "name": "Verteilte Konsensverfahren"
            },
            {
              "name": "Cloud-Computing und Cloud-Infrastrukturen"
            },
            {
              "name": "Cluster- und Grid-Computing"
            },
            {
              "name": "Edge-Computing"
            },
            {
              "name": "Dezentrale und gleichrangige Systeme"
            },
            {
              "name": "Verteilte Speichersysteme"
            },
            {
              "name": "Dienstorientierte und mikrodienstbasierte Systeme"
            }
          ]
        }
      ]
    },
    {
      "name": "Physik",
      "nodes": [
        {
          "name": "Mechanik",
          "children": [
            {
              "name": "Kinematik"
            },
            {
              "name": "Statik"
            },
            {
              "name": "Dynamik"
            },
            {
              "name": "Analytische Mechanik"
            },
            {
              "name": "Mechanik starrer Körper"
            },
            {
              "name": "Kontinuumsmechanik"
            },
            {
              "name": "Festkörpermechanik"
            },
            {
              "name": "Fluidmechanik"
            },
            {
              "name": "Nichtlineare Dynamik"
            },
            {
              "name": "Himmelsmechanik"
            }
          ]
        },
        {
          "name": "Thermodynamik und statistische Physik",
          "children": [
            {
              "name": "Klassische Thermodynamik"
            },
            {
              "name": "Statistische Mechanik"
            },
            {
              "name": "Kinetische Gastheorie"
            },
            {
              "name": "Nichtgleichgewichtsthermodynamik"
            },
            {
              "name": "Wärmeübertragung"
            },
            {
              "name": "Phasenübergänge und kritische Phänomene"
            },
            {
              "name": "Tieftemperaturphysik"
            }
          ]
        },
        {
          "name": "Elektromagnetismus",
          "children": [
            {
              "name": "Elektrostatik"
            },
            {
              "name": "Magnetostatik"
            },
            {
              "name": "Elektrodynamik"
            },
            {
              "name": "Elektromagnetische Wellenausbreitung"
            },
            {
              "name": "Elektrodynamik kontinuierlicher Medien"
            },
            {
              "name": "Hochfrequenzphysik"
            },
            {
              "name": "Plasmaphysik"
            },
            {
              "name": "Magnetohydrodynamik"
            }
          ]
        },
        {
          "name": "Optik",
          "children": [
            {
              "name": "Geometrische Optik"
            },
            {
              "name": "Wellenoptik"
            },
            {
              "name": "Quantenoptik"
            },
            {
              "name": "Nichtlineare Optik"
            },
            {
              "name": "Laserphysik"
            },
            {
              "name": "Photonik"
            },
            {
              "name": "Ultrakurzzeitoptik"
            }
          ]
        },
        {
          "name": "Akustik und Schwingungslehre",
          "children": [
            {
              "name": "Schwingungstheorie"
            },
            {
              "name": "Wellentheorie"
            },
            {
              "name": "Physikalische Akustik"
            },
            {
              "name": "Technische Akustik"
            },
            {
              "name": "Raumakustik"
            },
            {
              "name": "Ultraschallphysik"
            }
          ]
        },
        {
          "name": "Quantenphysik",
          "children": [
            {
              "name": "Nichtrelativistische Quantenmechanik"
            },
            {
              "name": "Relativistische Quantenmechanik"
            },
            {
              "name": "Quantenfeldtheorie"
            },
            {
              "name": "Quantenvielteilchentheorie"
            },
            {
              "name": "Quanteninformationstheorie"
            },
            {
              "name": "Quantenmesstheorie"
            }
          ]
        },
        {
          "name": "Relativitätstheorie und Gravitation",
          "children": [
            {
              "name": "Spezielle Relativitätstheorie"
            },
            {
              "name": "Allgemeine Relativitätstheorie"
            },
            {
              "name": "Experimentelle Gravitationsphysik"
            },
            {
              "name": "Gravitationswellenphysik"
            },
            {
              "name": "Relativistische Astrophysik"
            },
            {
              "name": "Relativistische Kosmologie"
            },
            {
              "name": "Quantengravitation"
            }
          ]
        },
        {
          "name": "Atom- und Molekülphysik",
          "children": [
            {
              "name": "Atomphysik"
            },
            {
              "name": "Molekülphysik"
            },
            {
              "name": "Atom- und Molekülspektroskopie"
            },
            {
              "name": "Atomare und molekulare Stoßphysik"
            },
            {
              "name": "Physik ultrakalter Atome und Moleküle"
            },
            {
              "name": "Clusterphysik"
            }
          ]
        },
        {
          "name": "Kern- und Teilchenphysik",
          "children": [
            {
              "name": "Kernstrukturphysik"
            },
            {
              "name": "Kernreaktionsphysik"
            },
            {
              "name": "Kernzerfallsphysik"
            },
            {
              "name": "Kernastrophysik"
            },
            {
              "name": "Theoretische Teilchenphysik"
            },
            {
              "name": "Experimentelle Teilchenphysik"
            },
            {
              "name": "Hadronenphysik"
            },
            {
              "name": "Astroteilchenphysik"
            },
            {
              "name": "Beschleunigerphysik"
            }
          ]
        }
      ]
    },
    {
      "name": "Chemie",
      "nodes": [
        {
          "name": "Allgemeine Chemie",
          "children": [
            {
              "name": "Aufbau der Atome"
            },
            {
              "name": "Periodensystem der Elemente"
            },
            {
              "name": "Chemische Bindungen"
            },
            {
              "name": "Molekülstruktur und chemische Strukturformeln"
            },
            {
              "name": "Chemische Reaktionen und Reaktionsgleichungen"
            },
            {
              "name": "Stöchiometrie"
            },
            {
              "name": "Aggregatzustände und Stoffeigenschaften"
            },
            {
              "name": "Lösungen und Löslichkeit"
            },
            {
              "name": "Chemische Gleichgewichte"
            },
            {
              "name": "Säure-Base-Chemie"
            },
            {
              "name": "Redoxchemie"
            },
            {
              "name": "Chemische Nomenklatur"
            }
          ]
        },
        {
          "name": "Organische Chemie",
          "children": [
            {
              "name": "Organische Synthesechemie"
            },
            {
              "name": "Physikalisch-organische Chemie"
            },
            {
              "name": "Stereochemie"
            },
            {
              "name": "Heterocyclenchemie"
            },
            {
              "name": "Naturstoffchemie"
            },
            {
              "name": "Polymerchemie"
            },
            {
              "name": "Medizinische Chemie"
            },
            {
              "name": "Supramolekulare Chemie"
            }
          ]
        },
        {
          "name": "Anorganische Chemie",
          "children": [
            {
              "name": "Hauptgruppenchemie"
            },
            {
              "name": "Übergangsmetallchemie"
            },
            {
              "name": "Koordinationschemie"
            },
            {
              "name": "Metallorganische Chemie"
            },
            {
              "name": "Festkörperchemie"
            },
            {
              "name": "Anorganische Synthesechemie"
            },
            {
              "name": "Bioanorganische Chemie"
            },
            {
              "name": "Radiochemie"
            }
          ]
        },
        {
          "name": "Physikalische Chemie",
          "children": [
            {
              "name": "Chemische Thermodynamik"
            },
            {
              "name": "Chemische Kinetik und Reaktionsdynamik"
            },
            {
              "name": "Quantenchemie"
            },
            {
              "name": "Elektrochemie"
            },
            {
              "name": "Oberflächen- und Grenzflächenchemie"
            },
            {
              "name": "Kolloidchemie"
            },
            {
              "name": "Photochemie"
            },
            {
              "name": "Molekülspektroskopie"
            },
            {
              "name": "Theoretische und Computerchemie"
            }
          ]
        },
        {
          "name": "Analytische Chemie",
          "children": [
            {
              "name": "Klassische chemische Analytik"
            },
            {
              "name": "Spektroskopische Analytik"
            },
            {
              "name": "Massenspektrometrische Analytik"
            },
            {
              "name": "Chromatografische Analytik"
            },
            {
              "name": "Elektroanalytik"
            },
            {
              "name": "Thermische Analyse"
            },
            {
              "name": "Oberflächenanalytik"
            },
            {
              "name": "Strukturaufklärung"
            },
            {
              "name": "Chemometrie"
            },
            {
              "name": "Bioanalytik"
            }
          ]
        }
      ]
    },
    {
      "name": "Biologie",
      "nodes": [
        {
          "name": "Allgemeine Biologie",
          "children": [
            {
              "name": "Theoretische Biologie"
            },
            {
              "name": "Systembiologie"
            },
            {
              "name": "Biologische Systematik und Taxonomie"
            },
            {
              "name": "Bioinformatik"
            },
            {
              "name": "Synthetische Biologie"
            },
            {
              "name": "Chronobiologie"
            }
          ]
        },
        {
          "name": "Anatomie und Physiologie",
          "children": [
            {
              "name": "Makroskopische Anatomie"
            },
            {
              "name": "Mikroskopische Anatomie und Histologie"
            },
            {
              "name": "Funktionelle Anatomie"
            },
            {
              "name": "Kreislaufphysiologie"
            },
            {
              "name": "Atmungsphysiologie"
            },
            {
              "name": "Verdauungsphysiologie"
            },
            {
              "name": "Nieren- und Osmoregulationsphysiologie"
            },
            {
              "name": "Endokrinologie"
            },
            {
              "name": "Muskelphysiologie"
            },
            {
              "name": "Bewegungsphysiologie"
            },
            {
              "name": "Ernährungsphysiologie"
            },
            {
              "name": "Reproduktionsphysiologie"
            },
            {
              "name": "Pathophysiologie"
            }
          ]
        },
        {
          "name": "Zellbiologie",
          "children": [
            {
              "name": "Zellmorphologie und Ultrastruktur"
            },
            {
              "name": "Membranbiologie"
            },
            {
              "name": "Organellenbiologie"
            },
            {
              "name": "Zytoskelettbiologie und Zellbewegung"
            },
            {
              "name": "Zellzyklus und Zellteilung"
            },
            {
              "name": "Zelluläre Signaltransduktion"
            },
            {
              "name": "Zelladhäsion und Zellkontakte"
            },
            {
              "name": "Intrazellulärer Transport und Membranverkehr"
            },
            {
              "name": "Zelluläre Stressbiologie"
            },
            {
              "name": "Zelltod und zelluläre Alterung"
            },
            {
              "name": "Stammzellbiologie"
            }
          ]
        },
        {
          "name": "Biochemie",
          "children": [
            {
              "name": "Strukturelle Biochemie"
            },
            {
              "name": "Proteinbiochemie"
            },
            {
              "name": "Enzymologie"
            },
            {
              "name": "Kohlenhydratbiochemie und Glykobiologie"
            },
            {
              "name": "Lipidbiochemie"
            },
            {
              "name": "Nukleinsäurebiochemie"
            },
            {
              "name": "Stoffwechselbiochemie"
            },
            {
              "name": "Bioenergetik"
            },
            {
              "name": "Proteomik"
            },
            {
              "name": "Metabolomik"
            }
          ]
        },
        {
          "name": "Molekularbiologie",
          "children": [
            {
              "name": "Genomorganisation und Chromatinbiologie"
            },
            {
              "name": "DNA-Replikation"
            },
            {
              "name": "DNA-Reparatur und Genomstabilität"
            },
            {
              "name": "Transkriptionsbiologie"
            },
            {
              "name": "RNA-Biologie und RNA-Prozessierung"
            },
            {
              "name": "Translation und Proteinbiosynthese"
            },
            {
              "name": "Molekulare Genregulation"
            },
            {
              "name": "Gentechnik und Genomeditierung"
            }
          ]
        },
        {
          "name": "Genetik",
          "children": [
            {
              "name": "Klassische Genetik"
            },
            {
              "name": "Funktionelle Genetik"
            },
            {
              "name": "Zytogenetik"
            },
            {
              "name": "Populationsgenetik"
            },
            {
              "name": "Quantitative Genetik"
            },
            {
              "name": "Epigenetik"
            },
            {
              "name": "Genomik"
            },
            {
              "name": "Humangenetik"
            },
            {
              "name": "Verhaltensgenetik"
            },
            {
              "name": "Pharmakogenetik und Pharmakogenomik"
            }
          ]
        },
        {
          "name": "Biophysik",
          "children": [
            {
              "name": "Molekulare und strukturelle Biophysik"
            },
            {
              "name": "Membranbiophysik"
            },
            {
              "name": "Zellbiophysik"
            },
            {
              "name": "Biomechanik"
            },
            {
              "name": "Elektrophysiologie"
            },
            {
              "name": "Statistische Biophysik"
            },
            {
              "name": "Biophotonik"
            },
            {
              "name": "Radiobiologie"
            },
            {
              "name": "Computergestützte Biophysik"
            }
          ]
        },
        {
          "name": "Mikrobiologie",
          "children": [
            {
              "name": "Bakteriologie"
            },
            {
              "name": "Archaeenbiologie"
            },
            {
              "name": "Protistologie"
            },
            {
              "name": "Mikrobielle Physiologie und Stoffwechselbiologie"
            },
            {
              "name": "Mikrobielle Genetik und Genomik"
            },
            {
              "name": "Mikrobielle Ökologie"
            },
            {
              "name": "Mikrobiomforschung"
            },
            {
              "name": "Medizinische Bakteriologie"
            },
            {
              "name": "Industrielle und angewandte Mikrobiologie"
            }
          ]
        },
        {
          "name": "Mykologie",
          "children": [
            {
              "name": "Pilzsystematik und Phylogenie"
            },
            {
              "name": "Pilzmorphologie und Anatomie"
            },
            {
              "name": "Pilzphysiologie"
            },
            {
              "name": "Pilzgenetik und Genomik"
            },
            {
              "name": "Pilzökologie"
            },
            {
              "name": "Flechtenkunde"
            },
            {
              "name": "Medizinische Mykologie"
            },
            {
              "name": "Angewandte Mykologie und Pilzbiotechnologie"
            }
          ]
        },
        {
          "name": "Virologie",
          "children": [
            {
              "name": "Virustaxonomie und Virusphylogenie"
            },
            {
              "name": "Strukturvirologie"
            },
            {
              "name": "Virusgenetik und Virusgenomik"
            },
            {
              "name": "Virusreplikation"
            },
            {
              "name": "Virus-Wirt-Interaktionen"
            },
            {
              "name": "Virusevolution"
            },
            {
              "name": "Virusökologie"
            },
            {
              "name": "Medizinische Virologie"
            },
            {
              "name": "Pflanzenvirologie"
            },
            {
              "name": "Bakteriophagenforschung"
            },
            {
              "name": "Angewandte Virologie"
            }
          ]
        },
        {
          "name": "Botanik",
          "children": [
            {
              "name": "Pflanzensystematik und Phylogenie"
            },
            {
              "name": "Pflanzenmorphologie und Anatomie"
            },
            {
              "name": "Pflanzenphysiologie"
            },
            {
              "name": "Pflanzenentwicklungsbiologie"
            },
            {
              "name": "Pflanzengenetik und Genomik"
            },
            {
              "name": "Pflanzenökologie"
            },
            {
              "name": "Geobotanik und Vegetationskunde"
            },
            {
              "name": "Pflanzenpathologie"
            },
            {
              "name": "Paläobotanik"
            },
            {
              "name": "Phykologie"
            },
            {
              "name": "Ethnobotanik"
            },
            {
              "name": "Pflanzenbiotechnologie"
            }
          ]
        },
        {
          "name": "Zoologie",
          "children": [
            {
              "name": "Tiersystematik und Phylogenie"
            },
            {
              "name": "Vergleichende Tiermorphologie und Anatomie"
            },
            {
              "name": "Vergleichende Tierphysiologie"
            },
            {
              "name": "Entomologie"
            },
            {
              "name": "Arachnologie"
            },
            {
              "name": "Krebstierkunde"
            },
            {
              "name": "Malakologie"
            },
            {
              "name": "Ichthyologie"
            },
            {
              "name": "Herpetologie"
            },
            {
              "name": "Ornithologie"
            },
            {
              "name": "Säugetierkunde"
            },
            {
              "name": "Ethologie"
            },
            {
              "name": "Parasitologie"
            },
            {
              "name": "Paläozoologie"
            }
          ]
        },
        {
          "name": "Ökologie",
          "children": [
            {
              "name": "Autökologie"
            },
            {
              "name": "Populationsökologie"
            },
            {
              "name": "Gemeinschaftsökologie"
            },
            {
              "name": "Ökosystemökologie"
            },
            {
              "name": "Biogeochemie"
            },
            {
              "name": "Landschaftsökologie"
            },
            {
              "name": "Makroökologie"
            },
            {
              "name": "Biogeografie"
            },
            {
              "name": "Verhaltensökologie"
            },
            {
              "name": "Chemische Ökologie"
            },
            {
              "name": "Terrestrische Ökologie"
            },
            {
              "name": "Limnologie"
            },
            {
              "name": "Meeresökologie"
            },
            {
              "name": "Biodiversitätsforschung"
            },
            {
              "name": "Naturschutzbiologie"
            },
            {
              "name": "Renaturierungsökologie"
            },
            {
              "name": "Ökotoxikologie"
            }
          ]
        },
        {
          "name": "Evolutionsbiologie",
          "children": [
            {
              "name": "Theoretische Evolutionsbiologie"
            },
            {
              "name": "Experimentelle Evolutionsbiologie"
            },
            {
              "name": "Molekulare Evolution"
            },
            {
              "name": "Phylogenetik"
            },
            {
              "name": "Artbildungsforschung"
            },
            {
              "name": "Evolutionsökologie"
            },
            {
              "name": "Evolutionäre Entwicklungsbiologie"
            },
            {
              "name": "Paläobiologie"
            }
          ]
        },
        {
          "name": "Entwicklungsbiologie",
          "children": [
            {
              "name": "Reproduktionsbiologie"
            },
            {
              "name": "Embryologie"
            },
            {
              "name": "Entwicklungsgenetik"
            },
            {
              "name": "Morphogenese und Zelldifferenzierung"
            },
            {
              "name": "Gewebe- und Organentwicklung"
            },
            {
              "name": "Regenerationsbiologie"
            },
            {
              "name": "Teratologie"
            }
          ]
        },
        {
          "name": "Neurowissenschaften",
          "children": [
            {
              "name": "Neuroanatomie"
            },
            {
              "name": "Molekulare und zelluläre Neurobiologie"
            },
            {
              "name": "Neurophysiologie"
            },
            {
              "name": "Systemneurowissenschaften"
            },
            {
              "name": "Sensorische Neurowissenschaften"
            },
            {
              "name": "Motorische Neurowissenschaften"
            },
            {
              "name": "Kognitive Neurowissenschaften"
            },
            {
              "name": "Affektive Neurowissenschaften"
            },
            {
              "name": "Verhaltensneurowissenschaften"
            },
            {
              "name": "Entwicklungsneurobiologie"
            },
            {
              "name": "Theoretische und computergestützte Neurowissenschaften"
            },
            {
              "name": "Neuroendokrinologie"
            },
            {
              "name": "Neuropharmakologie"
            },
            {
              "name": "Neuropathologie"
            }
          ]
        },
        {
          "name": "Immunologie",
          "children": [
            {
              "name": "Angeborene Immunität"
            },
            {
              "name": "Adaptive Immunität"
            },
            {
              "name": "Immunogenetik"
            },
            {
              "name": "Immunregulation und Immuntoleranz"
            },
            {
              "name": "Schleimhautimmunologie"
            },
            {
              "name": "Infektionsimmunologie"
            },
            {
              "name": "Tumorimmunologie"
            },
            {
              "name": "Autoimmunitätsforschung"
            },
            {
              "name": "Allergologie"
            },
            {
              "name": "Transplantationsimmunologie"
            },
            {
              "name": "Neuroimmunologie"
            },
            {
              "name": "Vergleichende Immunologie"
            },
            {
              "name": "Vakzinologie"
            },
            {
              "name": "Immuntherapie"
            }
          ]
        }
      ]
    },
    {
      "name": "Astronomie und Astrophysik",
      "nodes": [
        {
          "name": "Kosmologie",
          "children": []
        },
        {
          "name": "Stellarastronomie und Sternphysik",
          "children": []
        },
        {
          "name": "Planetologie",
          "children": []
        },
        {
          "name": "Galaktische und extragalaktische Astronomie",
          "children": []
        }
      ]
    },
    {
      "name": "Geowissenschaften",
      "nodes": [
        {
          "name": "Geologie",
          "children": []
        },
        {
          "name": "Geophysik",
          "children": []
        },
        {
          "name": "Geochemie",
          "children": []
        },
        {
          "name": "Mineralogie und Petrologie",
          "children": []
        },
        {
          "name": "Geomorphologie und Bodenkunde",
          "children": []
        },
        {
          "name": "Meteorologie und Klimatologie",
          "children": []
        },
        {
          "name": "Ozeanografie und Hydrologie",
          "children": []
        },
        {
          "name": "Glaziologie und Kryosphärenforschung",
          "children": []
        }
      ]
    },
    {
      "name": "Umweltwissenschaften",
      "nodes": [
        {
          "name": "Umweltbelastungen und Umweltveränderungen",
          "children": []
        },
        {
          "name": "Natur- und Biodiversitätsschutz",
          "children": []
        },
        {
          "name": "Nachhaltigkeit und Ressourcenmanagement",
          "children": []
        },
        {
          "name": "Umweltmonitoring und Umweltbewertung",
          "children": []
        }
      ]
    },
    {
      "name": "Medizin und Gesundheitswissenschaften",
      "nodes": [
        {
          "name": "Pathologie und Pathophysiologie",
          "children": []
        },
        {
          "name": "Ernährungswissenschaften",
          "children": []
        },
        {
          "name": "Innere Medizin",
          "children": []
        },
        {
          "name": "Kardiologie und Angiologie",
          "children": []
        },
        {
          "name": "Pneumologie",
          "children": []
        },
        {
          "name": "Gastroenterologie und Hepatologie",
          "children": []
        },
        {
          "name": "Endokrinologie und Diabetologie",
          "children": []
        },
        {
          "name": "Nephrologie und Urologie",
          "children": []
        },
        {
          "name": "Hämatologie und Onkologie",
          "children": []
        },
        {
          "name": "Chirurgie",
          "children": []
        },
        {
          "name": "Orthopädie und Unfallchirurgie",
          "children": []
        },
        {
          "name": "Neurologie",
          "children": []
        },
        {
          "name": "Kinder- und Jugendmedizin",
          "children": []
        },
        {
          "name": "Frauenheilkunde und Geburtshilfe",
          "children": []
        },
        {
          "name": "Dermatologie",
          "children": []
        },
        {
          "name": "Augenheilkunde",
          "children": []
        },
        {
          "name": "Hals-Nasen-Ohren-Heilkunde",
          "children": []
        },
        {
          "name": "Pharmakologie und Toxikologie",
          "children": []
        },
        {
          "name": "Rehabilitationsmedizin und Therapiewissenschaften",
          "children": []
        },
        {
          "name": "Zahnmedizin",
          "children": []
        },
        {
          "name": "Psychiatrie",
          "children": []
        },
        {
          "name": "Präventivmedizin und Public Health",
          "children": []
        }
      ]
    },
    {
      "name": "Technologie und Ingenieurwesen",
      "nodes": [
        {
          "name": "Maschinenbau",
          "children": []
        },
        {
          "name": "Elektrotechnik und Elektronik",
          "children": []
        },
        {
          "name": "Energietechnik",
          "children": []
        },
        {
          "name": "Mechatronik und Robotik",
          "children": []
        },
        {
          "name": "Werkstoffwissenschaft und Materialtechnik",
          "children": []
        },
        {
          "name": "Verfahrenstechnik",
          "children": []
        },
        {
          "name": "Systemtechnik und Systems Engineering",
          "children": []
        },
        {
          "name": "Architektur und Gebäudetechnik",
          "children": []
        },
        {
          "name": "Bauingenieurwesen",
          "children": []
        },
        {
          "name": "Fahrzeug- und Verkehrstechnik",
          "children": []
        },
        {
          "name": "Kommunikations- und Nachrichtentechnik",
          "children": []
        },
        {
          "name": "Produktionstechnik und Fertigung",
          "children": []
        },
        {
          "name": "Landwirtschaft und Agrartechnik",
          "children": []
        },
        {
          "name": "Medizintechnik",
          "children": []
        },
        {
          "name": "Biotechnologie",
          "children": []
        },
        {
          "name": "Umwelttechnik",
          "children": []
        }
      ]
    },
    {
      "name": "Psychologie",
      "nodes": [
        {
          "name": "Allgemeine Psychologie",
          "children": []
        },
        {
          "name": "Entwicklungspsychologie",
          "children": []
        },
        {
          "name": "Differentielle und Persönlichkeitspsychologie",
          "children": []
        },
        {
          "name": "Sozialpsychologie",
          "children": []
        },
        {
          "name": "Biologische Psychologie und Neuropsychologie",
          "children": []
        },
        {
          "name": "Klinische Psychologie und Psychotherapie",
          "children": []
        },
        {
          "name": "Pädagogische Psychologie",
          "children": []
        },
        {
          "name": "Arbeits- und Organisationspsychologie",
          "children": []
        },
        {
          "name": "Gesundheitspsychologie",
          "children": []
        },
        {
          "name": "Medien- und Kommunikationspsychologie",
          "children": []
        },
        {
          "name": "Rechtspsychologie",
          "children": []
        }
      ]
    },
    {
      "name": "Sozialwissenschaften",
      "nodes": [
        {
          "name": "Soziologie",
          "children": []
        },
        {
          "name": "Politikwissenschaft",
          "children": []
        },
        {
          "name": "Rechtswissenschaft",
          "children": []
        },
        {
          "name": "Kommunikations- und Medienwissenschaft",
          "children": []
        },
        {
          "name": "Kriminologie",
          "children": []
        },
        {
          "name": "Sozialpolitik und Sozialwesen",
          "children": []
        }
      ]
    },
    {
      "name": "Wirtschaftswissenschaften",
      "nodes": [
        {
          "name": "Volkswirtschaftslehre",
          "children": []
        },
        {
          "name": "Betriebswirtschaftslehre",
          "children": []
        },
        {
          "name": "Finanzwirtschaft",
          "children": []
        },
        {
          "name": "Produktions- und Logistikmanagement",
          "children": []
        },
        {
          "name": "Wirtschaftspolitik und öffentliche Finanzen",
          "children": []
        },
        {
          "name": "Internationale Wirtschaft",
          "children": []
        }
      ]
    },
    {
      "name": "Philosophie",
      "nodes": [
        {
          "name": "Metaphysik",
          "children": [
            {
              "name": "Ontologie"
            },
            {
              "name": "Modalmetaphysik"
            },
            {
              "name": "Raum- und Zeitphilosophie"
            },
            {
              "name": "Kausalitätsmetaphysik"
            },
            {
              "name": "Naturphilosophie"
            }
          ]
        },
        {
          "name": "Erkenntnistheorie",
          "children": [
            {
              "name": "Theorie des Wissens und der Rechtfertigung"
            },
            {
              "name": "Wahrheitstheorie"
            },
            {
              "name": "Formale Erkenntnistheorie"
            },
            {
              "name": "Soziale Erkenntnistheorie"
            },
            {
              "name": "Erkenntnistheorie der Wahrnehmung"
            }
          ]
        },
        {
          "name": "Logik",
          "children": [
            {
              "name": "Philosophie der Logik"
            },
            {
              "name": "Informelle Logik und Argumentationstheorie"
            }
          ]
        },
        {
          "name": "Ethik",
          "children": [
            {
              "name": "Metaethik"
            },
            {
              "name": "Normative Ethik"
            },
            {
              "name": "Angewandte Ethik"
            },
            {
              "name": "Moralpsychologie"
            }
          ]
        },
        {
          "name": "Ästhetik",
          "children": [
            {
              "name": "Kunstphilosophie"
            },
            {
              "name": "Philosophie der ästhetischen Wahrnehmung und Erfahrung"
            },
            {
              "name": "Naturästhetik"
            },
            {
              "name": "Alltagsästhetik"
            }
          ]
        },
        {
          "name": "Sprachphilosophie",
          "children": [
            {
              "name": "Philosophische Semantik"
            },
            {
              "name": "Philosophische Pragmatik"
            },
            {
              "name": "Philosophie des Sprachverstehens"
            }
          ]
        },
        {
          "name": "Wissenschaftsphilosophie",
          "children": [
            {
              "name": "Allgemeine Wissenschaftstheorie"
            },
            {
              "name": "Wissenschaftliche Methodologie"
            },
            {
              "name": "Philosophie der Mathematik"
            },
            {
              "name": "Philosophie der Physik"
            },
            {
              "name": "Philosophie der Chemie"
            },
            {
              "name": "Philosophie der Biologie"
            },
            {
              "name": "Philosophie der Sozialwissenschaften"
            },
            {
              "name": "Philosophie der Medizin"
            }
          ]
        },
        {
          "name": "Philosophie des Geistes",
          "children": [
            {
              "name": "Bewusstseinsphilosophie"
            },
            {
              "name": "Philosophie des Leib-Seele-Verhältnisses"
            },
            {
              "name": "Philosophie der Intentionalität"
            },
            {
              "name": "Philosophie der Wahrnehmung"
            },
            {
              "name": "Philosophie der Kognition"
            },
            {
              "name": "Philosophie der Emotionen"
            },
            {
              "name": "Philosophie der künstlichen Intelligenz"
            }
          ]
        },
        {
          "name": "Sozial- und politische Philosophie",
          "children": [
            {
              "name": "Sozialphilosophie"
            },
            {
              "name": "Politische Philosophie"
            },
            {
              "name": "Rechtsphilosophie"
            },
            {
              "name": "Kulturphilosophie"
            },
            {
              "name": "Geschichtsphilosophie"
            },
            {
              "name": "Wirtschaftsphilosophie"
            }
          ]
        },
        {
          "name": "Philosophische Anthropologie",
          "children": [
            {
              "name": "Philosophie der menschlichen Natur"
            },
            {
              "name": "Existenzphilosophie"
            },
            {
              "name": "Handlungsphilosophie"
            },
            {
              "name": "Freiheitsphilosophie"
            },
            {
              "name": "Philosophie der Person"
            }
          ]
        }
      ]
    },
    {
      "name": "Religion",
      "nodes": [
        {
          "name": "Religionswissenschaft",
          "children": []
        },
        {
          "name": "Religionsphilosophie",
          "children": []
        },
        {
          "name": "Christentum",
          "children": []
        },
        {
          "name": "Islam",
          "children": []
        },
        {
          "name": "Judentum",
          "children": []
        },
        {
          "name": "Hinduismus",
          "children": []
        },
        {
          "name": "Buddhismus",
          "children": []
        },
        {
          "name": "Indigene und traditionelle Religionen",
          "children": []
        },
        {
          "name": "Historische Religionen und Mythologien",
          "children": []
        },
        {
          "name": "Weitere Religionen und religiöse Bewegungen",
          "children": []
        }
      ]
    },
    {
      "name": "Geografie",
      "nodes": [
        {
          "name": "Physische Geografie",
          "children": []
        },
        {
          "name": "Humangeografie",
          "children": []
        },
        {
          "name": "Kartografie und Geoinformationssysteme",
          "children": []
        },
        {
          "name": "Regionalgeografie und Länderkunde",
          "children": []
        }
      ]
    },
    {
      "name": "Kunst",
      "nodes": [
        {
          "name": "Kunsttheorie",
          "children": [
            {
              "name": "Farbtheorie"
            },
            {
              "name": "Formen- und Kompositionslehre"
            },
            {
              "name": "Kunstkritik und Kunstinterpretation"
            },
            {
              "name": "Ikonografie und Ikonologie"
            },
            {
              "name": "Bildwissenschaft"
            }
          ]
        },
        {
          "name": "Zeichnung und Malerei",
          "children": [
            {
              "name": "Künstlerische Zeichnung"
            },
            {
              "name": "Malerei"
            },
            {
              "name": "Illustration"
            }
          ]
        },
        {
          "name": "Musik",
          "children": [
            {
              "name": "Musikwissenschaft und Musikgeschichte"
            },
            {
              "name": "Musiktheorie"
            },
            {
              "name": "Instrumentalspiel"
            },
            {
              "name": "Gesang und Stimmbildung"
            }
          ]
        },
        {
          "name": "Tanz",
          "children": [
            {
              "name": "Tanztechnik und Körpertraining"
            },
            {
              "name": "Choreografie"
            },
            {
              "name": "Urbaner Tanz"
            }
          ]
        }
      ]
    },
    {
      "name": "Sport und Bewegung",
      "nodes": [
        {
          "name": "Sportwissenschaft",
          "children": [
            {
              "name": "Trainingswissenschaft"
            },
            {
              "name": "Bewegungswissenschaft"
            },
            {
              "name": "Sportbiomechanik"
            },
            {
              "name": "Sportphysiologie"
            },
            {
              "name": "Sportpsychologie"
            }
          ]
        },
        {
          "name": "Kraft",
          "children": [
            {
              "name": "Allgemeines Krafttraining"
            },
            {
              "name": "Bodybuilding"
            },
            {
              "name": "Calisthenics"
            },
            {
              "name": "Schnellkraft- und Sprungkrafttraining"
            },
            {
              "name": "Funktionelles Fitnesstraining"
            }
          ]
        },
        {
          "name": "Ausdauer",
          "children": [
            {
              "name": "Laufen"
            },
            {
              "name": "Radfahren"
            },
            {
              "name": "Schwimmen"
            }
          ]
        },
        {
          "name": "Beweglichkeit und Körperbeherrschung",
          "children": [
            {
              "name": "Beweglichkeitstraining"
            },
            {
              "name": "Koordinations- und Gleichgewichtstraining"
            },
            {
              "name": "Körperstabilisation und Haltungskontrolle"
            },
            {
              "name": "Akrobatik"
            }
          ]
        }
      ]
    },
    {
      "name": "Geschichte",
      "nodes": []
    },
    {
      "name": "Sprache und Kommunikation",
      "nodes": []
    }
  ]
}$misa_json$::JSONB;
    v_domain JSONB;
    v_parent JSONB;
    v_child JSONB;
    v_domain_id domains.id%TYPE;
    v_parent_id domain_nodes.id%TYPE;
BEGIN
FOR v_domain IN SELECT value FROM jsonb_array_elements(v_data->'domains') LOOP
    IF jsonb_array_length(v_domain->'nodes') = 0 THEN
            CONTINUE;
END IF;

SELECT d.id INTO STRICT v_domain_id
FROM domains AS d
WHERE d.name = v_domain->>'name';

FOR v_parent IN SELECT value FROM jsonb_array_elements(v_domain->'nodes') LOOP
    IF NOT EXISTS (
                SELECT 1 FROM domain_nodes
                WHERE domain_id = v_domain_id
                  AND parent_id IS NULL
                  AND name = v_parent->>'name'
            ) THEN
                INSERT INTO domain_nodes (domain_id, parent_id, name, is_system_value, cognitive_level_id)
                VALUES (v_domain_id, NULL, v_parent->>'name', TRUE, 1)
                    RETURNING id INTO v_parent_id;
ELSE
SELECT n.id INTO STRICT v_parent_id
FROM domain_nodes AS n
WHERE n.domain_id = v_domain_id
  AND n.parent_id IS NULL
  AND n.name = v_parent->>'name';
END IF;

FOR v_child IN SELECT value FROM jsonb_array_elements(v_parent->'children') LOOP
    IF NOT EXISTS (
                    SELECT 1 FROM domain_nodes
                    WHERE domain_id = v_domain_id
                      AND parent_id = v_parent_id
                      AND name = v_child->>'name'
                ) THEN
               INSERT INTO domain_nodes (domain_id, parent_id, name, is_system_value, cognitive_level_id)
               VALUES (v_domain_id, v_parent_id, v_child->>'name', TRUE, 1);
END IF;
END LOOP;
END LOOP;
END LOOP;
END;
$misa_import$;

COMMIT;

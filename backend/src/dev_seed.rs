use crate::models::entity::EntityType;
use crate::repositories::entity_repository;
use chrono::{Duration, Utc};
use sqlx::{PgPool, Postgres, Transaction};

const TASKS: [(&str, &str); 24] = [
    ("Wocheneinkauf planen", "Einkaufsliste prüfen und ergänzen"),
    ("Wohnung aufräumen", "Küche, Bad und Wohnzimmer ordnen"),
    (
        "Mathematik lernen",
        "Übungsaufgaben für die nächste Klausur lösen",
    ),
    (
        "Arzttermin vereinbaren",
        "Vorsorgetermin telefonisch abstimmen",
    ),
    (
        "Fahrrad reparieren",
        "Bremsen und Beleuchtung kontrollieren",
    ),
    (
        "Präsentation vorbereiten",
        "Folien erstellen und Vortrag üben",
    ),
    (
        "Geburtstagsgeschenk besorgen",
        "Geschenk auswählen und einpacken",
    ),
    ("Unterlagen sortieren", "Wichtige Dokumente digital ablegen"),
    (
        "Wochenplan erstellen",
        "Termine und Aufgaben für die Woche planen",
    ),
    (
        "Deutschaufsatz überarbeiten",
        "Text korrigieren und sauber abschreiben",
    ),
    (
        "Kühlschrank reinigen",
        "Lebensmittel prüfen und Fächer auswischen",
    ),
    ("Versicherung anrufen", "Offene Fragen zum Vertrag klären"),
    (
        "Laufschuhe kaufen",
        "Passende Schuhe im Sportgeschäft auswählen",
    ),
    (
        "Bücher zurückgeben",
        "Ausgeliehene Bücher zur Bibliothek bringen",
    ),
    ("Rezept ausprobieren", "Ein neues Abendessen kochen"),
    ("Budget aktualisieren", "Ausgaben des Monats eintragen"),
    ("Pflanzen umtopfen", "Neue Erde und größere Töpfe verwenden"),
    (
        "Englischvokabeln lernen",
        "Die aktuelle Lektion wiederholen",
    ),
    (
        "Schreibtisch organisieren",
        "Arbeitsplatz aufräumen und Kabel ordnen",
    ),
    ("Familienbesuch planen", "Termin und Anreise abstimmen"),
    (
        "Fotos sichern",
        "Neue Bilder auf dem Backup-Laufwerk speichern",
    ),
    (
        "Steuerunterlagen sammeln",
        "Belege und Bescheinigungen zusammenstellen",
    ),
    (
        "Kleiderschrank aussortieren",
        "Nicht mehr benötigte Kleidung spenden",
    ),
    (
        "Projektbericht schreiben",
        "Ergebnisse zusammenfassen und prüfen",
    ),
];

const APPOINTMENTS: [&str; 12] = [
    "Zahnarzttermin",
    "Treffen mit Freunden",
    "Elternabend",
    "Sportkurs",
    "Lernrunde",
    "Frühstück im Café",
    "Werkstatttermin",
    "Telefonat mit der Familie",
    "Bibliotheksbesuch",
    "Spaziergang",
    "Wochenplanung",
    "Konzertbesuch",
];

const REMINDERS: [&str; 8] = [
    "Mülltonne herausstellen",
    "Medikament einnehmen",
    "Rechnung bezahlen",
    "Pflanzen gießen",
    "Rückruf erledigen",
    "Paket abholen",
    "Brot aus dem Ofen nehmen",
    "Geburtstag nicht vergessen",
];

const TOPICS: [(&str, Option<usize>, &str); 40] = [
    (
        "Softwareentwicklung",
        None,
        "Überblick über die Entwicklung wartbarer Software.",
    ),
    (
        "Frontend",
        Some(0),
        "Grundlagen moderner Benutzeroberflächen.",
    ),
    (
        "HTML & CSS",
        Some(1),
        "Semantisches HTML und responsive Layouts.",
    ),
    (
        "React & TypeScript",
        Some(1),
        "Typsichere Komponenten und Zustandsverwaltung.",
    ),
    (
        "Backend",
        Some(0),
        "Serverlogik, Schnittstellen und Persistenz.",
    ),
    (
        "Rust",
        Some(4),
        "Ownership, Typen und asynchrone Anwendungen.",
    ),
    (
        "REST APIs",
        Some(4),
        "Ressourcen, HTTP-Methoden und Statuscodes.",
    ),
    (
        "Qualität & Architektur",
        Some(0),
        "Strukturen für robuste Softwaresysteme.",
    ),
    (
        "Testing",
        Some(7),
        "Unit-, Integrations- und End-to-End-Tests.",
    ),
    (
        "Softwarearchitektur",
        Some(7),
        "Module, Abhängigkeiten und Systemgrenzen.",
    ),
    (
        "Mathematik",
        None,
        "Mathematische Grundlagen und Problemlösungsverfahren.",
    ),
    (
        "Analysis",
        Some(10),
        "Funktionen, Grenzwerte und Veränderungen.",
    ),
    (
        "Differentialrechnung",
        Some(11),
        "Ableitungen und lokale Änderungsraten.",
    ),
    (
        "Integralrechnung",
        Some(11),
        "Flächen, Stammfunktionen und Integrale.",
    ),
    (
        "Lineare Algebra",
        Some(10),
        "Vektorräume und lineare Abbildungen.",
    ),
    (
        "Vektoren & Matrizen",
        Some(14),
        "Rechenoperationen mit Vektoren und Matrizen.",
    ),
    (
        "Lineare Gleichungssysteme",
        Some(14),
        "Lösungsverfahren für lineare Gleichungen.",
    ),
    (
        "Diskrete Mathematik",
        Some(10),
        "Endliche Strukturen und formale Methoden.",
    ),
    (
        "Logik & Mengenlehre",
        Some(17),
        "Aussagenlogik, Mengen und Relationen.",
    ),
    (
        "Graphentheorie",
        Some(17),
        "Knoten, Kanten, Wege und Bäume.",
    ),
    (
        "Persönliche Produktivität",
        None,
        "Methoden für bewusstes und wirksames Arbeiten.",
    ),
    (
        "Planung",
        Some(20),
        "Aufgaben und Zeit realistisch strukturieren.",
    ),
    (
        "Ziele & Prioritäten",
        Some(21),
        "Ziele formulieren und Wichtiges auswählen.",
    ),
    (
        "Wochenplanung",
        Some(21),
        "Eine Woche mit Aufgaben und Terminen vorbereiten.",
    ),
    (
        "Fokus",
        Some(20),
        "Aufmerksamkeit gezielt einsetzen und schützen.",
    ),
    (
        "Deep Work",
        Some(24),
        "Konzentrierte Arbeit ohne Unterbrechungen.",
    ),
    (
        "Ablenkungsmanagement",
        Some(24),
        "Störungen erkennen und systematisch reduzieren.",
    ),
    (
        "Reflexion",
        Some(20),
        "Erfahrungen auswerten und Verhalten verbessern.",
    ),
    (
        "Gewohnheiten",
        Some(27),
        "Hilfreiche Routinen aufbauen und festigen.",
    ),
    (
        "Reviews & Journaling",
        Some(27),
        "Fortschritt regelmäßig schriftlich reflektieren.",
    ),
    (
        "Finanzen",
        None,
        "Grundlagen für nachhaltige persönliche Finanzen.",
    ),
    (
        "Finanzielle Grundlagen",
        Some(30),
        "Einnahmen, Ausgaben und Rücklagen verstehen.",
    ),
    (
        "Budgetierung",
        Some(31),
        "Geldflüsse planen und kontrollieren.",
    ),
    (
        "Notgroschen",
        Some(31),
        "Eine Reserve für unerwartete Ausgaben aufbauen.",
    ),
    (
        "Investieren",
        Some(30),
        "Langfristiger Vermögensaufbau mit Wertpapieren.",
    ),
    ("ETFs", Some(34), "Breit gestreute Indexfonds verstehen."),
    (
        "Risiko & Diversifikation",
        Some(34),
        "Anlagerisiken verteilen und bewerten.",
    ),
    (
        "Vorsorge",
        Some(30),
        "Langfristige finanzielle Risiken absichern.",
    ),
    (
        "Versicherungen",
        Some(37),
        "Wichtige Risiken passend absichern.",
    ),
    (
        "Altersvorsorge",
        Some(37),
        "Versorgungslücken erkennen und langfristig planen.",
    ),
];

pub async fn seed(pool: &PgPool) -> Result<(), sqlx::Error> {
    let mut tx = pool.begin().await?;

    sqlx::query("DELETE FROM document_revisions")
        .execute(&mut *tx)
        .await?;
    sqlx::query("DELETE FROM documents")
        .execute(&mut *tx)
        .await?;
    sqlx::query("DELETE FROM topics").execute(&mut *tx).await?;

    sqlx::query("DELETE FROM entities WHERE type IN ($1, $2, $3, $4)")
        .bind(EntityType::Task as i32)
        .bind(EntityType::CalendarEntry as i32)
        .bind(EntityType::Topic as i32)
        .bind(EntityType::Document as i32)
        .execute(&mut *tx)
        .await?;

    let task_ids = seed_tasks(&mut tx).await?;
    seed_calendar_entries(&mut tx, &task_ids).await?;
    seed_topics(&mut tx).await?;

    tx.commit().await
}

async fn seed_topics(tx: &mut Transaction<'_, Postgres>) -> Result<(), sqlx::Error> {
    let mut topic_ids = Vec::with_capacity(TOPICS.len());

    for (index, (name, parent_index, content)) in TOPICS.iter().enumerate() {
        let topic_entity_id = entity_repository::create_entity(tx, EntityType::Topic).await?;
        let parent_topic_entity_id = parent_index.map(|index| topic_ids[index]);

        sqlx::query(
            "INSERT INTO topics
                (entity_id, parent_topic_entity_id, name, understanding_level_id)
             VALUES ($1, $2, $3, $4)",
        )
        .bind(topic_entity_id)
        .bind(parent_topic_entity_id)
        .bind(name)
        .bind((index % 5 + 1) as i32)
        .execute(&mut **tx)
        .await?;

        let document_entity_id = entity_repository::create_entity(tx, EntityType::Document).await?;
        sqlx::query(
            "INSERT INTO documents (entity_id, topic_id, content)
             VALUES ($1, $2, $3)",
        )
        .bind(document_entity_id)
        .bind(topic_entity_id)
        .bind(content)
        .execute(&mut **tx)
        .await?;

        topic_ids.push(topic_entity_id);
    }

    Ok(())
}

async fn seed_tasks(tx: &mut Transaction<'_, Postgres>) -> Result<Vec<i32>, sqlx::Error> {
    let group_ids = sqlx::query_scalar::<_, i32>("SELECT id FROM task_groups ORDER BY id")
        .fetch_all(&mut **tx)
        .await?;
    let state_ids = sqlx::query_scalar::<_, i32>("SELECT id FROM task_states ORDER BY id")
        .fetch_all(&mut **tx)
        .await?;
    let priority_ids = sqlx::query_scalar::<_, i32>("SELECT id FROM task_priorities ORDER BY id")
        .fetch_all(&mut **tx)
        .await?;

    if group_ids.is_empty() || state_ids.is_empty() || priority_ids.is_empty() {
        return Err(sqlx::Error::RowNotFound);
    }

    let now = Utc::now();
    let mut task_ids = Vec::with_capacity(TASKS.len());

    for (index, (name, description)) in TASKS.iter().enumerate() {
        let entity_id = entity_repository::create_entity(tx, EntityType::Task).await?;
        let state_id = state_ids[index % state_ids.len()];
        let completed_at = (state_id == 3).then_some(now - Duration::days(1));

        sqlx::query(
            "INSERT INTO tasks
                (entity_id, group_id, name, description, state_id, priority_id,
                 due_at, completed_at)
             VALUES ($1, $2, $3, $4, $5, $6, $7, $8)",
        )
        .bind(entity_id)
        .bind(group_ids[index % group_ids.len()])
        .bind(name)
        .bind(description)
        .bind(state_id)
        .bind(priority_ids[(index * 3) % priority_ids.len()])
        .bind(Some(now + Duration::days((index as i64 % 18) - 4)))
        .bind(completed_at)
        .execute(&mut **tx)
        .await?;

        task_ids.push(entity_id);
    }

    Ok(task_ids)
}

async fn seed_calendar_entries(
    tx: &mut Transaction<'_, Postgres>,
    task_ids: &[i32],
) -> Result<(), sqlx::Error> {
    let now = Utc::now();

    for index in 0..70 {
        let entity_id = entity_repository::create_entity(tx, EntityType::CalendarEntry).await?;
        let day_offset = if index < 56 {
            ((index * 11) % 29) as i64 - 14
        } else {
            ((index * 17) % 85) as i64 - 42
        };
        let start_at =
            now + Duration::days(day_offset) + Duration::hours(((index * 5) % 17) as i64 - 8);
        let is_reminder = index % 6 == 0;
        let end_at =
            (!is_reminder).then_some(start_at + Duration::minutes(30 + (index % 4) as i64 * 30));
        let source_entity_id = (index < 42).then_some(task_ids[index % task_ids.len()]);
        let (name, note) = if is_reminder {
            (
                REMINDERS[index % REMINDERS.len()].to_string(),
                Some("Kurze Erinnerung für den Alltag".to_string()),
            )
        } else if let Some(task_id) = source_entity_id {
            let task_index = task_ids.iter().position(|id| *id == task_id).unwrap_or(0);
            (
                format!("Zeit für {}", TASKS[task_index].0),
                Some("Für diese Aufgabe reservierter Zeitraum".to_string()),
            )
        } else {
            (
                APPOINTMENTS[index % APPOINTMENTS.len()].to_string(),
                Some("Eigenständiger Kalendereintrag für Testzwecke".to_string()),
            )
        };

        sqlx::query(
            "INSERT INTO calendar_entries
                (entity_id, source_entity_id, name, note, is_reminder, start_at, end_at)
             VALUES ($1, $2, $3, $4, $5, $6, $7)",
        )
        .bind(entity_id)
        .bind(source_entity_id)
        .bind(name)
        .bind(note)
        .bind(is_reminder)
        .bind(start_at)
        .bind(end_at)
        .execute(&mut **tx)
        .await?;
    }

    Ok(())
}

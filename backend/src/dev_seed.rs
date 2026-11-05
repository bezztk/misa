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

pub async fn seed(pool: &PgPool) -> Result<(), sqlx::Error> {
    let mut tx = pool.begin().await?;

    sqlx::query("DELETE FROM entities WHERE type = $1 OR type = $2")
        .bind(EntityType::Task as i32)
        .bind(EntityType::CalendarEntry as i32)
        .execute(&mut *tx)
        .await?;

    let task_ids = seed_tasks(&mut tx).await?;
    seed_calendar_entries(&mut tx, &task_ids).await?;

    tx.commit().await
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
        let (title, note) = if is_reminder {
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
                (entity_id, source_entity_id, title, note, is_reminder, start_at, end_at)
             VALUES ($1, $2, $3, $4, $5, $6, $7)",
        )
        .bind(entity_id)
        .bind(source_entity_id)
        .bind(title)
        .bind(note)
        .bind(is_reminder)
        .bind(start_at)
        .bind(end_at)
        .execute(&mut **tx)
        .await?;
    }

    Ok(())
}

use crate::models::metadata::Metadata;
use sqlx::PgPool;
use sqlx::types::Json;
use tokio::sync::OnceCell;

static METADATA: OnceCell<Metadata> = OnceCell::const_new();

pub async fn get_metadata(pool: &PgPool) -> Result<Metadata, sqlx::Error> {
    Ok(METADATA
        .get_or_try_init(|| load_metadata(pool))
        .await?
        .clone())
}

async fn load_metadata(pool: &PgPool) -> Result<Metadata, sqlx::Error> {
    let Json(metadata) = sqlx::query_scalar::<_, Json<Metadata>>(
        "SELECT jsonb_build_object(
             'personal_relevance_levels',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM personal_relevance_levels) value),
             'lasting_impacts',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM lasting_impacts) value),
             'knowledge_levels',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name, description FROM knowledge_levels) value),
             'psychomotor_levels',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM psychomotor_levels) value),
             'document_completeness_levels',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM document_completeness_levels) value),
             'document_flashcard_statuses',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM document_flashcard_statuses) value),
             'document_verification_statuses',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM document_verification_statuses) value),
             'document_verification_levels',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM document_verification_levels) value),
             'flashcard_retention_classes',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name,
                               desired_retention::REAL AS desired_retention
                        FROM flashcard_retention_classes) value),
             'flashcard_review_states',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name FROM flashcard_review_states) value),
             'idea_phases',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name, description FROM idea_phases) value),
             'idea_states',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name, description FROM idea_states) value),
             'media_ratings',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM media_ratings) value),
             'pursuit_requirement_types',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name FROM pursuit_requirement_types) value),
             'systemtask_groups',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name FROM systemtask_groups) value),
             'systemtask_states',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM systemtask_states) value),
             'systemtask_priorities',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM systemtask_priorities) value),
             'systemtask_routine_execution_states',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name
                        FROM systemtask_routine_execution_states) value),
             'systemtask_session_presets',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, planned_duration_minutes
                        FROM systemtask_session_presets) value),
             'systemtask_session_states',
                 (SELECT COALESCE(jsonb_agg(value ORDER BY id), '[]'::jsonb)
                  FROM (SELECT id, name, display_name FROM systemtask_session_states) value)
         )",
    )
    .fetch_one(pool)
    .await?;

    Ok(metadata)
}

use crate::api::handlers::{
    attachment_handler, calendar_entry_handler, comment_handler, document_handler, domain_handler,
    flashcard_handler, metadata_handler, progression_handler, pursuit_handler, routine_handler,
    skill_handler, task_handler,
};
use axum::{Router, routing::get};
use sqlx::PgPool;

pub fn create_router(pool: PgPool) -> Router {
    let api_routes = Router::new()
        .route(
            "/routines/generate-executions",
            axum::routing::post(routine_handler::generate_executions),
        )
        .route(
            "/calendar-entries",
            get(calendar_entry_handler::get_calendar_entries)
                .post(calendar_entry_handler::create_calendar_entry),
        )
        .route(
            "/calendar-entries/{id}",
            axum::routing::patch(calendar_entry_handler::update_calendar_entry)
                .delete(calendar_entry_handler::delete_calendar_entry),
        )
        .route(
            "/entities/{entity_id}/comments",
            axum::routing::post(comment_handler::create_comment),
        )
        .route(
            "/comments/{id}",
            axum::routing::patch(comment_handler::update_comment)
                .delete(comment_handler::delete_comment),
        )
        .route("/domain-tree", get(domain_handler::get_domain_tree))
        .route(
            "/domain-nodes/{parent_id}/children",
            axum::routing::post(domain_handler::create_domain_node),
        )
        .route(
            "/domains/{domain_id}/nodes",
            axum::routing::post(domain_handler::create_root_domain_node),
        )
        .route(
            "/domain-nodes/{id}",
            axum::routing::patch(domain_handler::update_domain_node)
                .delete(domain_handler::delete_domain_node),
        )
        .route(
            "/domain-nodes/{id}/fields/{field_id}",
            axum::routing::patch(domain_handler::update_domain_node_field),
        )
        .route(
            "/domain-nodes/{node_id}/document",
            get(document_handler::get_document)
                .post(document_handler::create_document)
                .put(document_handler::update_document)
                .delete(document_handler::delete_document),
        )
        .route(
            "/flashcards/due",
            get(flashcard_handler::get_due_flashcards),
        )
        .route(
            "/flashcards/{id}",
            axum::routing::patch(flashcard_handler::update_flashcard)
                .delete(flashcard_handler::delete_flashcard),
        )
        .route(
            "/flashcards/{id}/reviews",
            axum::routing::post(flashcard_handler::review_flashcard),
        )
        .route(
            "/domain-nodes/{node_id}/flashcards",
            get(flashcard_handler::get_flashcards).post(flashcard_handler::create_flashcard),
        )
        .route(
            "/domain-nodes/{node_id}/attachments",
            get(attachment_handler::get_attachments).post(attachment_handler::create_attachment),
        )
        .route(
            "/attachments/{id}",
            axum::routing::delete(attachment_handler::delete_attachment),
        )
        .route(
            "/domain-nodes/{node_id}/skills",
            get(skill_handler::get_skills).post(skill_handler::create_skill),
        )
        .route(
            "/domain-nodes/{node_id}/pursuits",
            get(pursuit_handler::get_pursuits).post(pursuit_handler::create_pursuit),
        )
        .route(
            "/pursuits/{id}/requirements",
            axum::routing::put(pursuit_handler::update_requirements),
        )
        .route(
            "/progressions",
            get(progression_handler::get_progressions)
                .post(progression_handler::create_progression),
        )
        .route(
            "/progressions/{id}",
            axum::routing::patch(progression_handler::update_progression),
        )
        .route(
            "/progressions/{id}/stages",
            axum::routing::post(progression_handler::create_stage),
        )
        .route(
            "/pursuits/available",
            get(progression_handler::get_available_pursuits),
        )
        .route(
            "/progression-stages/{id}/pursuits",
            axum::routing::post(progression_handler::assign_pursuit),
        )
        .route(
            "/skills/{id}",
            axum::routing::patch(skill_handler::update_skill),
        )
        .route("/metadata", get(metadata_handler::get_metadata))
        .route(
            "/tasks/{id}",
            axum::routing::patch(task_handler::update_task).delete(task_handler::delete_task),
        )
        .route(
            "/tasks/{id}/fields/{field_id}",
            axum::routing::patch(task_handler::update_task_field),
        )
        .route(
            "/tasks",
            get(task_handler::get_tasks).post(task_handler::create_task),
        );

    Router::new()
        .route("/health", get(health))
        .nest("/api", api_routes)
        .with_state(pool)
}

async fn health() -> &'static str {
    "OK"
}

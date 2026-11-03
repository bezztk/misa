use crate::api::handlers::{
    development_area_handler, development_category_handler, goal_handler, item_handler,
    skill_handler, task_handler,
};
use axum::{Router, routing::{get, patch}};
use sqlx::PgPool;

pub fn create_router(pool: PgPool) -> Router {
    let api_routes = Router::new()
        .route("/items/hierarchy", get(item_handler::get_item_hierarchy))
        .route("/items/{id}/parent", patch(item_handler::update_item_parent))
        .route("/items/{id}/archive", patch(item_handler::update_item_archive))
        .route(
            "/goals/{id}",
            get(goal_handler::get_goal)
                .patch(goal_handler::update_goal)
                .delete(goal_handler::delete_goal),
        )
        .route(
            "/goals",
            get(goal_handler::get_goals).post(goal_handler::create_goal),
        )
        .route(
            "/skills/{id}",
            get(skill_handler::get_skill)
                .patch(skill_handler::update_skill)
                .delete(skill_handler::delete_skill),
        )
        .route(
            "/skills",
            get(skill_handler::get_skills).post(skill_handler::create_skill),
        )
        .route("/skills/metadata", get(skill_handler::get_skill_metadata))
        .route(
            "/development-categories/{id}",
            get(development_category_handler::get_development_category)
                .patch(development_category_handler::update_development_category)
                .delete(development_category_handler::delete_development_category),
        )
        .route(
            "/development-categories",
            get(development_category_handler::get_development_categories)
                .post(development_category_handler::create_development_category),
        )
        .route(
            "/development-areas/{id}",
            get(development_area_handler::get_development_area)
                .patch(development_area_handler::update_development_area)
                .delete(development_area_handler::delete_development_area),
        )
        .route(
            "/development-areas",
            get(development_area_handler::get_development_areas)
                .post(development_area_handler::create_development_area),
        )
        .route(
            "/tasks/{id}",
            get(task_handler::get_task)
                .patch(task_handler::update_task)
                .delete(task_handler::delete_task),
        )
        .route(
            "/tasks",
            get(task_handler::get_tasks).post(task_handler::create_task),
        )
        .route("/tasks/metadata", get(task_handler::get_task_metadata));

    Router::new()
        .route("/health", get(health))
        .nest("/api", api_routes)
        .with_state(pool)
}

async fn health() -> &'static str {
    "OK"
}

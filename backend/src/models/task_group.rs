#[derive(Debug, sqlx::FromRow)]
pub struct TaskGroup {
    pub id: i32,
    pub name: String,
}
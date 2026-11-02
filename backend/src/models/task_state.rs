#[derive(Debug, sqlx::FromRow)]
pub struct TaskStateRow {
    pub id: i32,
    pub name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TaskState {
    Open = 1,
    InProgress = 2,
    Completed = 3,
    Cancelled = 4,
}
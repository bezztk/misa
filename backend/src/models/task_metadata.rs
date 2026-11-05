use serde::Serialize;

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct TaskGroup {
    pub id: i32,
    pub name: String,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct TaskStateRow {
    pub id: i32,
    pub name: String,
}

#[derive(Debug, sqlx::FromRow, Serialize)]
pub struct TaskPriority {
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

#[derive(Debug, Serialize)]
pub struct TaskMetadata {
    pub groups: Vec<TaskGroup>,
    pub states: Vec<TaskStateRow>,
    pub priorities: Vec<TaskPriority>,
}

#[derive(Debug, sqlx::FromRow)]
pub struct ItemTypeRow {
    pub id: i32,
    pub name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ItemType {
    Task = 1
}
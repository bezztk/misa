#[derive(Debug, sqlx::FromRow)]
pub struct ItemTypeRow {
    pub id: i32,
    pub name: String,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ItemType {
    Task = 1,
    DevelopmentCategory = 2,
    Goal = 3,
    Skill = 4,
}

impl ItemType {
    pub const fn allowed_parent_types(self) -> &'static [ItemType] {
        match self {
            ItemType::Task => &[
                ItemType::DevelopmentCategory,
                ItemType::Goal,
                ItemType::Skill,
            ],

            ItemType::Goal => &[
                ItemType::DevelopmentCategory,
                ItemType::Skill,
                ItemType::Goal,
            ],

            ItemType::Skill => &[
                ItemType::DevelopmentCategory,
                ItemType::Skill,
            ],

            ItemType::DevelopmentCategory => &[
                ItemType::DevelopmentCategory,
                ItemType::Goal,
                ItemType::Skill,
            ],
        }
    }
}

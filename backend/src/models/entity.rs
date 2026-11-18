#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EntityType {
    DomainNode = 1,
    SystemTask = 2,
    Progression = 3,
    Pursuit = 4,
    Skill = 5,
    Flashcard = 6,
    CalendarEntry = 7,
    Attachment = 8,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EntityType {
    Task = 1,
    CalendarEntry = 2,
    Topic = 3,
    Document = 4,
}

use serde::{Deserialize, Serialize};

macro_rules! lookup_model {
    ($name:ident) => {
        #[derive(Debug, Clone, PartialEq, Eq, sqlx::FromRow, Serialize, Deserialize)]
        pub struct $name {
            pub id: i32,
            pub name: String,
            pub display_name: String,
        }
    };
}

lookup_model!(PersonalRelevanceLevel);
lookup_model!(LastingImpact);
lookup_model!(CognitiveLevel);
lookup_model!(PsychomotorLevel);
lookup_model!(DocumentCompletenessLevel);
lookup_model!(DocumentFlashcardStatus);
lookup_model!(DocumentVerificationStatus);
lookup_model!(DocumentVerificationLevel);

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum PersonalRelevanceLevelId {
    Insignificant = 1,
    Secondary = 2,
    Significant = 3,
    Essential = 4,
    IdentityDefining = 5,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum LastingImpactId {
    Negligible = 1,
    Minor = 2,
    Noticeable = 3,
    Profound = 4,
    Transformative = 5,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum CognitiveLevelId {
    Remember = 1,
    Understand = 2,
    Apply = 3,
    Analyze = 4,
    Evaluate = 5,
    Create = 6,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum PsychomotorLevelId {
    Imitation = 1,
    Manipulation = 2,
    Precision = 3,
    Articulation = 4,
    Naturalization = 5,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum DocumentCompletenessLevelId {
    None = 1,
    Partial = 2,
    Substantial = 3,
    Complete = 4,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum DocumentFlashcardStatusId {
    Pending = 1,
    PartiallyCompleted = 2,
    Completed = 3,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum DocumentVerificationStatusId {
    Unverified = 1,
    PartiallyVerified = 2,
    Verified = 3,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum DocumentVerificationLevelId {
    SelfReviewed = 1,
    CrossChecked = 2,
    Validated = 3,
}

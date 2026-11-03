export interface Skill {
    item_id: number;
    area_id: number;
    state_id: number;
    name: string;
    description: string | null;
}

export interface CreateSkill {
    area_id: number;
    state_id: number | null;
    name: string;
    description: string | null;
}

export interface UpdateSkill {
    state_id?: number | null;
    name?: string;
    description?: string | null;
}

export interface SkillState {
    id: number;
    name: string;
}

export interface SkillMetadata {
    states: SkillState[];
}

export interface Goal {
    item_id: number;
    area_id: number;
    name: string;
    description: string | null;
    is_completed: boolean;
    completed_at: string | null;
    due_at: string | null;
}

export interface CreateGoal {
    area_id: number;
    name: string;
    description: string | null;
    due_at: string | null;
}

export interface UpdateGoal {
    name?: string;
    description?: string | null;
    is_completed?: boolean;
    due_at?: string;
    clear_due_at?: boolean;
}

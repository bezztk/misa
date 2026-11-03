export interface Task {
    item_id: number;
    item_type: number;
    created_at: string;
    updated_at: string;
    is_archived: boolean;
    version: number;

    group_id: number | null;
    state_id: number | null;
    name: string;
    description: string | null;
    completed_at: string | null;
}

export interface CreateTask {
    name: string;
    description: string | null;
    group_id: number | null;
}

export interface UpdateTask {
    name?: string;
    description?: string | null;
    group_id?: number | null;
    state_id?: number | null;
}

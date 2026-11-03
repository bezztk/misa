export interface TaskMetadata {
    groups: TaskGroup[];
    states: TaskState[];
}

export interface TaskGroup {
    id: number;
    name: string;
}

export interface TaskState {
    id: number;
    name: string;
}
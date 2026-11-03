export interface DevelopmentCategory {
    item_id: number;
    area_id: number;
    name: string;
}

export interface CreateDevelopmentCategory {
    area_id: number;
    name: string;
}

export interface UpdateDevelopmentCategory {
    name?: string;
}

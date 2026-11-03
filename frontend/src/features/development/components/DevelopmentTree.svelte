<script lang="ts">
    import {onMount} from "svelte";
    import {getDevelopmentCategories} from "../../../lib/api/developmentCategories";
    import {getGoals} from "../../../lib/api/goals";
    import {getItemHierarchy} from "../../../lib/api/items";
    import {getSkills} from "../../../lib/api/skills";
    import {getTasks} from "../../../lib/api/tasks";

    let {areaId}: {areaId: number} = $props();

    interface TreeItem {
        id: number;
        name: string;
        type: "Category" | "Skill" | "Goal" | "Task";
        parentId: number | null;
    }

    interface TreeRow extends TreeItem {
        depth: number;
    }

    let rows: TreeRow[] = $state([]);
    let error = $state("");

    async function loadTree() {
        try {
            error = "";

            const [categories, skills, goals, tasks, hierarchy] = await Promise.all([
                getDevelopmentCategories(),
                getSkills(),
                getGoals(),
                getTasks(),
                getItemHierarchy(),
            ]);

            const items: TreeItem[] = [
                ...categories
                    .filter(category => category.area_id === areaId)
                    .map(category => ({
                        id: category.item_id,
                        name: category.name,
                        type: "Category" as const,
                        parentId: null,
                    })),
                ...skills
                    .filter(skill => skill.area_id === areaId)
                    .map(skill => ({
                        id: skill.item_id,
                        name: skill.name,
                        type: "Skill" as const,
                        parentId: null,
                    })),
                ...goals
                    .filter(goal => goal.area_id === areaId)
                    .map(goal => ({
                        id: goal.item_id,
                        name: goal.name,
                        type: "Goal" as const,
                        parentId: null,
                    })),
                ...tasks
                    .filter(task => task.area_id === areaId)
                    .map(task => ({
                        id: task.item_id,
                        name: task.name,
                        type: "Task" as const,
                        parentId: null,
                    })),
            ];

            const itemIds = new Set(items.map(item => item.id));
            const parents = new Map(hierarchy.map(item => [item.child_id, item.parent_id]));

            for (const item of items) {
                const parentId = parents.get(item.id);
                item.parentId = parentId !== undefined && itemIds.has(parentId) ? parentId : null;
            }

            items.sort((a, b) => a.name.localeCompare(b.name));

            const nextRows: TreeRow[] = [];

            function addChildren(parentId: number | null, depth: number) {
                for (const item of items.filter(item => item.parentId === parentId)) {
                    nextRows.push({...item, depth});
                    addChildren(item.id, depth + 1);
                }
            }

            addChildren(null, 0);
            rows = nextRows;
        } catch (err) {
            error = String(err);
        }
    }

    onMount(() => {
        void loadTree();
    });
</script>

<section>
    <h3>Hierarchie</h3>

    {#if error}
        <p>{error}</p>
    {:else if rows.length === 0}
        <p>Keine Elemente vorhanden.</p>
    {:else}
        {#each rows as row (row.id)}
            <div style={`padding-left: ${row.depth * 1.5}rem`}>
                {row.name} ({row.type})
            </div>
        {/each}
    {/if}
</section>

<style>
    div {
        margin: 0.25rem 0;
    }
</style>

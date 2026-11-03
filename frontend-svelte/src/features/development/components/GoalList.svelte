<script lang="ts">
    import {onMount} from "svelte";
    import {createGoal, deleteGoal, getGoals, updateGoal} from "../../../lib/api/goals";
    import type {Goal} from "../../../lib/models/goal";

    let {areaId}: {areaId: number} = $props();

    let goals: Goal[] = $state([]);
    let editingGoal: Goal | null = $state(null);
    let name = $state("");
    let description = $state("");
    let isCompleted = $state(false);
    let dueDate = $state("");
    let error = $state("");
    let formError = $state("");
    let pending = $state(false);
    let dialog: HTMLDialogElement;

    async function loadGoals() {
        try {
            error = "";
            goals = (await getGoals()).filter(goal => goal.area_id === areaId);
        } catch (err) {
            error = String(err);
        }
    }

    function openCreate() {
        error = "";
        formError = "";
        editingGoal = null;
        name = "";
        description = "";
        isCompleted = false;
        dueDate = "";
        dialog.showModal();
    }

    function openEdit(goal: Goal) {
        error = "";
        formError = "";
        editingGoal = goal;
        name = goal.name;
        description = goal.description ?? "";
        isCompleted = goal.is_completed;
        dueDate = goal.due_at?.slice(0, 10) ?? "";
        dialog.showModal();
    }

    async function submit() {
        if (pending) return;

        try {
            formError = "";
            pending = true;

            const dueAt = dueDate
                ? new Date(`${dueDate}T00:00:00.000Z`).toISOString()
                : null;

            if (editingGoal) {
                await updateGoal(editingGoal.item_id, {
                    name,
                    description,
                    is_completed: isCompleted,
                    due_at: dueAt ?? undefined,
                    clear_due_at: dueAt === null,
                });
            } else {
                await createGoal({area_id: areaId, name, description, due_at: dueAt});
            }

            dialog.close();
            await loadGoals();
        } catch (err) {
            formError = String(err);
        } finally {
            pending = false;
        }
    }

    async function remove(goal: Goal) {
        if (pending || !confirm(`Goal "${goal.name}" löschen?`)) return;

        try {
            error = "";
            pending = true;
            await deleteGoal(goal.item_id);
            await loadGoals();
        } catch (err) {
            error = String(err);
        } finally {
            pending = false;
        }
    }

    onMount(() => {
        void loadGoals();
    });
</script>

<section>
    <h3>Goals</h3>
    <button onclick={openCreate} disabled={pending}>Neues Goal</button>

    {#if error}
        <p>{error}</p>
    {/if}

    {#each goals as goal (goal.item_id)}
        <article>
            <strong>{goal.name}</strong>
            <span>{goal.is_completed ? "Abgeschlossen" : "Offen"}</span>

            {#if goal.due_at}
                <span>Fällig: {new Date(goal.due_at).toLocaleDateString("de-DE")}</span>
            {/if}

            <button onclick={() => openEdit(goal)} disabled={pending}>Bearbeiten</button>
            <button onclick={() => remove(goal)} disabled={pending}>Löschen</button>

            {#if goal.description}
                <p>{goal.description}</p>
            {/if}
        </article>
    {/each}
</section>

<dialog bind:this={dialog}>
    <form onsubmit={(event) => {
        event.preventDefault();
        void submit();
    }}>
        <h3>{editingGoal ? "Goal bearbeiten" : "Neues Goal"}</h3>

        <label for="goal-name">Name</label>
        <input id="goal-name" bind:value={name} required />

        <label for="goal-description">Beschreibung</label>
        <textarea id="goal-description" bind:value={description}></textarea>

        <label for="goal-due-date">Fälligkeitsdatum</label>
        <input id="goal-due-date" type="date" bind:value={dueDate} />

        {#if editingGoal}
            <label>
                <input type="checkbox" bind:checked={isCompleted} />
                Abgeschlossen
            </label>
        {/if}

        {#if formError}
            <p>{formError}</p>
        {/if}

        <button type="submit" disabled={pending}>
            {pending ? "Wird gespeichert..." : "Speichern"}
        </button>
        <button type="button" onclick={() => dialog.close()}>Abbrechen</button>
    </form>
</dialog>

<style>
    article {
        padding: 1rem 0;
        border-bottom: 1px solid #ddd;
    }

    article span,
    article button {
        margin-left: 0.5rem;
    }

    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>

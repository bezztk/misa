<script lang="ts">
    import {createTask} from "../../../lib/api/tasks";
    import type {CreateTask, Task} from "../../../lib/models/task";

    let name = $state("");
    let description = $state("");
    let error = $state("");

    async function submit() {
        try {
            error = "";

            await createTask({
                name,
                description,
                group_id: null
            });

            name = "";
            description = "";
        } catch (err) {
            error = String(err);
        }
    }
</script>

<form onsubmit={(event) => {
    event.preventDefault();
    submit();
}}>
    <input type="text" placeholder="Aufgabenname" bind:value={name} required />
    <textarea placeholder="Beschreibung" bind:value={description}></textarea>
    <button type="submit">Aufgabe erstellen</button>
</form>

{#if error}
    <p>{error}</p>
{/if}
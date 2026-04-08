# Project Tracker Frontend

Vue 3 + TypeScript frontend for the Project-Tracker core business flow.

## Scope

Current pages:

- `/login`
- `/projects`
- `/projects/:projectId`
- `/my-tasks`

Current capabilities:

- Login and session bootstrap
- Project list with create/edit dialog and lifecycle action entry points
- Project workspace with summary, members, node rail, subtasks, member add/remove, subtask progress, and progress history drawer
- My tasks board for the current user

## Development

Install dependencies:

```bash
npm install
```

Start the dev server:

```bash
npm run dev
```

The Vite dev server proxies `/api` to `http://127.0.0.1:8080`.

## Verification

Run tests:

```bash
npm test
```

Build the production bundle:

```bash
npm run build
```

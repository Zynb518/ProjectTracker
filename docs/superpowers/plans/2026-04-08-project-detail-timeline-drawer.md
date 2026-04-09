# Project Detail Timeline Drawer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn the project detail lower workspace into a Git-style node timeline with hover details, a middle drawer for node subtasks, and a right-side member panel that stays visible.

**Architecture:** Keep existing project detail data loading and subtask APIs, but move node selection into an explicit drawer interaction. Refactor `NodeRail` into a timeline component, add a focused `NodeDrawer` wrapper around node overview plus `SubtaskTable`, and update the detail view to host the drawer while leaving the member panel in place.

**Tech Stack:** Vue 3, TypeScript, Pinia, Vue Testing Library, Vue Test Utils, Vitest, Vite

---

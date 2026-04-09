from __future__ import annotations

from typing import List, Optional

from pydantic import BaseModel, Field, field_validator


class ProjectDraft(BaseModel):
    name: str
    description: str = ""
    planned_start_date: str
    planned_end_date: str


class SubTaskDraft(BaseModel):
    name: str
    description: str = ""
    responsible_user_id: Optional[int] = None
    priority: int = Field(2)
    planned_start_date: str
    planned_end_date: str

    @field_validator("priority")
    @classmethod
    def _validate_priority(cls, value: int) -> int:
        if value not in {1, 2, 3}:
            raise ValueError("priority must be 1, 2, or 3")
        return value


class ProjectNodeDraft(BaseModel):
    name: str
    description: str = ""
    planned_start_date: str
    planned_end_date: str
    subtasks: List[SubTaskDraft] = Field(default_factory=list)


class GeneratedPlan(BaseModel):
    project: ProjectDraft
    nodes: List[ProjectNodeDraft] = Field(default_factory=list)

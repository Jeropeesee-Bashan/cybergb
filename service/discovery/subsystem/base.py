from abc import ABC, abstractmethod
from collections.abc import AsyncIterator, MutableMapping
from enum import StrEnum
from typing import ClassVar, TypedDict
from typing_extensions import override

from pydantic import BaseModel


Event = StrEnum("Event", ("connected", "disconnected"))


class Output(TypedDict):
    event: Event
    params: BaseModel


class Base(ABC):
    name: ClassVar[str]
    ParamsModel: ClassVar[type[BaseModel]]
    _registry: ClassVar[MutableMapping[str, type["Base"]]] = {}

    @abstractmethod
    def __aiter__(self) -> AsyncIterator[Output]:
        pass

    @override
    def __init_subclass__(cls):
        if "name" not in cls.__dict__:
            raise TypeError(f"{cls} has no attribute 'name'")

        if cls.name in cls._registry:
            raise TypeError(
                f"Subsystem name '{cls.name}' has alrready been registered by {cls.from_name(cls.name)}"
            )

        if "ParamsModel" not in cls.__dict__:
            raise TypeError(f"{cls} has no attribute 'ParamsModel'")

        cls._registry[cls.name] = cls

    @override
    def __init__(self, params: BaseModel):
        self._params: BaseModel = params

    @classmethod
    def from_name(cls, name: str) -> type["Base"]:
        return cls._registry[name]

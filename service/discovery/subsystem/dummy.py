import asyncio
from collections.abc import AsyncGenerator
from typing import final

from pydantic import BaseModel
from typing_extensions import override

from .base import Base, Event, Output


@final
class Dummy(Base):
    name = "dummy"

    class ParamsModel(BaseModel):
        device_id: str

    @override
    async def __aiter__(self) -> AsyncGenerator[Output]:
        while True:
            yield Output(
                event=self._event,
                params=self._params,
            )
            _ = await self._changed.wait()
            self._changed.clear()

    @override
    def __init__(self, params: "Dummy.ParamsModel"):
        super().__init__(params)

        self._event = Event.connected
        self._changed = asyncio.Event()

    @property
    def event(self) -> Event:
        return self._event

    @event.setter
    def event(self, value: Event):
        if self._event == value:
            return

        self._changed.set()
        self._event = value

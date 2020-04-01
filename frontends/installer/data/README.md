To make an installer, run cmake install, then
``candle.exe .\dynamo_3ds_max.wxs``
``torch.exe -ext WixUIExtension .\dynamo_3ds_max.wixobj``
from the ``dynamo_3ds_max_installer`` directory

Where ``candle.exe`` and ``torch.exe`` are part of the WIX toolkit.

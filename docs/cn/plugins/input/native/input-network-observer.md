# input_network_observer 插件

## 简介

`input_network_observer`插件可以实现利用ebpf探针采集网络可观测数据。

## 版本

[Dev](../../stability-level.md)

## 版本说明

* 推荐版本：【待发布】

## 配置参数

|  **参数**  |  **类型**  |  **是否必填**  |  **默认值**  |  **说明**  |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  是  |  /  |  插件类型。固定为input\_network\_observer  |
|  ProbeConfig  |  object  |  是  |  /  |  插件配置参数列表  |
|  ProbeConfig.EnableLog  |  bool  |  否  |  true  |  是否开启日志上报  |
|  ProbeConfig.EnableSpan  |  bool  |  否  |  false  |  是否开启跨度上报  |
|  ProbeConfig.EnableMetric  |  bool  |  否  |  false  |  是否开启指标上报  |
|  ProbeConfig.EnableProtocols  |  \[string\]  |  否  |  空  |  允许的协议类型  |
|  ProbeConfig.DisableProtocolParse  |  bool  |  否  |  false  |  TODO  |
|  ProbeConfig.DisableConnStats  |  bool  |  否  |  false  |  TODO  |
|  ProbeConfig.EnableConnTrackerDump  |  bool  |  否  |  false  |  TODO  |

## 样例

### XXXX

* 输入

```json
TODO
```

* 采集配置

```yaml
enable: true
inputs:
  - Type: input_network_observer
    ProbeConfig:
      EnableLog: true
      EnableMetric: false
      EnableSpan: false
      EnableProtocols: 
        - "http"
      DisableConnStats: false
      EnableConnTrackerDump: false
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

* 输出

```json
TODO
```

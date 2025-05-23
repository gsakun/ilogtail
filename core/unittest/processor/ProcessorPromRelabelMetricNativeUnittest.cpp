/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TextParser.h"
#include "common/JsonUtil.h"
#include "common/StringTools.h"
#include "models/MetricEvent.h"
#include "plugin/processor/inner/ProcessorPromRelabelMetricNative.h"
#include "prometheus/Constants.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {
class ProcessorPromRelabelMetricNativeUnittest : public testing::Test {
public:
    void SetUp() override { mContext.SetConfigName("project##config_0"); }

    void TestInit();
    void TestProcess();
    void TestAddAutoMetrics();
    void TestHonorLabels();

    CollectionPipelineContext mContext;
};

void ProcessorPromRelabelMetricNativeUnittest::TestInit() {
    Json::Value config;
    ProcessorPromRelabelMetricNative processor;
    processor.SetContext(mContext);

    // success config
    string configStr;
    string errorMsg;
    configStr = R"JSON(
        {
            "job_name": "test_job",
            "metric_relabel_configs": [
                {
                    "action": "keep",
                    "regex": "node-exporter",
                    "replacement": "$1",
                    "separator": ";",
                    "source_labels": [
                        "__meta_kubernetes_pod_label_app"
                    ]
                },
                {
                    "action": "replace",
                    "regex": "(.*)",
                    "replacement": "${1}:9100",
                    "separator": ";",
                    "source_labels": [
                        "__meta_kubernetes_pod_ip"
                    ],
                    "target_label": "__address__"
                }
            ]
        }
    )JSON";

    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));
    APSARA_TEST_TRUE(processor.Init(config));
}

void ProcessorPromRelabelMetricNativeUnittest::TestProcess() {
    // make config
    Json::Value config;

    ProcessorPromRelabelMetricNative processor;
    processor.SetContext(mContext);

    string configStr;
    string errorMsg;
    configStr = configStr + R"(
        {
            "job_name": "test_job",
            "metric_relabel_configs": [
                {
                    "action": "drop",
                    "regex": "v.*",
                    "replacement": "$1",
                    "separator": ";",
                    "source_labels": [
                        "k3"
                    ]
                },
                {
                    "action": "replace",
                    "regex": "(.*)"
        + ")\",\n" +
        R"(
                    "replacement": "${1}:9100",
                    "separator": ";",
                    "source_labels": [
                        "__meta_kubernetes_pod_ip"
                    ],
                    "target_label": "__address__"
                }
            ],
            "external_labels": {
                "test_key1": "test_value1",
                "test_key2": "test_value2",
                "test_key3": ""
            }
        }
    )";

    // init
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));
    APSARA_TEST_TRUE(processor.Init(config));

    // make events
    auto parser = TextParser();
    string rawData = R"""(
# begin

test_metric1{k1="v1", k2="v2"} 1.0
  test_metric2{k1="v1", k2="v2"} 2.0 1234567890
test_metric3{k1="v1",k2="v2"} 9.9410452992e+10
  test_metric4{k1="v1",k2="v2"} 9.9410452992e+10 1715829785083
  test_metric5{k1="v1", k2="v2" } 9.9410452992e+10 1715829785083
test_metric6{k1="v1",k2="v2",} 9.9410452992e+10 1715829785083
test_metric7{k1="v1",k3="", } 9.9410452992e+10 1715829785083  
test_metric8{k1="v1", k3="v2", } 9.9410452992e+10 1715829785083

# end
    )""";
    auto eventGroup = parser.Parse(rawData, 0, 0);

    // run function
    std::string pluginId = "testID";
    APSARA_TEST_EQUAL((size_t)8, eventGroup.GetEvents().size());
    processor.Process(eventGroup);

    // judge result
    APSARA_TEST_EQUAL((size_t)7, eventGroup.GetEvents().size());
    APSARA_TEST_EQUAL("test_metric1", eventGroup.GetEvents().at(0).Cast<MetricEvent>().GetName());
    APSARA_TEST_EQUAL("test_metric2", eventGroup.GetEvents().at(1).Cast<MetricEvent>().GetName());
    APSARA_TEST_EQUAL("test_metric3", eventGroup.GetEvents().at(2).Cast<MetricEvent>().GetName());
    APSARA_TEST_EQUAL("test_metric4", eventGroup.GetEvents().at(3).Cast<MetricEvent>().GetName());
    APSARA_TEST_EQUAL("test_metric5", eventGroup.GetEvents().at(4).Cast<MetricEvent>().GetName());
    APSARA_TEST_EQUAL("test_metric6", eventGroup.GetEvents().at(5).Cast<MetricEvent>().GetName());
    APSARA_TEST_EQUAL("test_metric7", eventGroup.GetEvents().at(6).Cast<MetricEvent>().GetName());
    // test_metric7 k3 label is removed because value is empty
    APSARA_TEST_EQUAL(false, eventGroup.GetEvents().at(6).Cast<MetricEvent>().HasTag("k3"));
    // test_metric8 is dropped by relabel config

    // check external labels
    APSARA_TEST_EQUAL("test_value1", eventGroup.GetEvents().at(0).Cast<MetricEvent>().GetTag("test_key1"));
    APSARA_TEST_EQUAL("test_value2", eventGroup.GetEvents().at(0).Cast<MetricEvent>().GetTag("test_key2"));
    APSARA_TEST_EQUAL(false, eventGroup.GetEvents().at(0).Cast<MetricEvent>().HasTag("test_key3"));
}

void ProcessorPromRelabelMetricNativeUnittest::TestAddAutoMetrics() {
    // make config
    Json::Value config;

    ProcessorPromRelabelMetricNative processor;
    processor.SetContext(mContext);

    string configStr;
    string errorMsg;
    configStr = configStr + R"(
        {
            "job_name": "test_job",
            "scrape_timeout": "15s",
            "sample_limit": 1000,
            "series_limit": 1000
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));

    // init
    APSARA_TEST_TRUE(processor.Init(config));

    // make events
    auto parser = TextParser();
    auto eventGroup = parser.Parse(R"""(
# begin
test_metric1{k1="v1", k2="v2"} 1.0
  test_metric2{k1="v1", k2="v2"} 2.0 1234567890
test_metric3{k1="v1",k2="v2"} 9.9410452992e+10
  test_metric4{k1="v1",k2="v2"} 9.9410452992e+10 1715829785083
  test_metric5{k1="v1", k2="v2" } 9.9410452992e+10 1715829785083
test_metric6{k1="v1",k2="v2",} 9.9410452992e+10 1715829785083
test_metric7{k1="v1",k3="2", } 9.9410452992e+10 1715829785083  
test_metric8{k1="v1", k3="v2", } 9.9410452992e+10 1715829785083
# end
    )""",
                                   0,
                                   0);

    APSARA_TEST_EQUAL((size_t)8, eventGroup.GetEvents().size());

    // without metadata
    auto autoMetric = prom::AutoMetric();
    processor.UpdateAutoMetrics(eventGroup, autoMetric);
    processor.AddAutoMetrics(eventGroup, autoMetric);
    APSARA_TEST_EQUAL((size_t)8, eventGroup.GetEvents().size());

    // with metadata
    eventGroup.SetMetadata(EventGroupMetaKey::PROMETHEUS_SCRAPE_TIMESTAMP_MILLISEC, ToString(1715829785083));
    eventGroup.SetMetadata(EventGroupMetaKey::PROMETHEUS_SAMPLES_SCRAPED, ToString(8));
    eventGroup.SetMetadata(EventGroupMetaKey::PROMETHEUS_SCRAPE_DURATION, ToString(1.5));
    eventGroup.SetMetadata(EventGroupMetaKey::PROMETHEUS_SCRAPE_RESPONSE_SIZE, ToString(2325));
    eventGroup.SetMetadata(EventGroupMetaKey::PROMETHEUS_UP_STATE, ToString(true));
    eventGroup.SetMetadata(EventGroupMetaKey::PROMETHEUS_SCRAPE_STATE, string("OK"));
    eventGroup.SetMetadata(EventGroupMetaKey::PROMETHEUS_STREAM_ID, string("123"));
    eventGroup.SetTag(string("instance"), "localhost:8080");
    eventGroup.SetTag(string("job"), "test_job");
    processor.UpdateAutoMetrics(eventGroup, autoMetric);
    processor.AddAutoMetrics(eventGroup, autoMetric);

    // SCRAPE_SAMPLES_POST_METRIC_RELABELING is removed
    APSARA_TEST_EQUAL((size_t)15, eventGroup.GetEvents().size());
    APSARA_TEST_EQUAL(1.5, eventGroup.GetEvents().at(8).Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue);
    APSARA_TEST_EQUAL(2325, eventGroup.GetEvents().at(9).Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue);
    APSARA_TEST_EQUAL(1000, eventGroup.GetEvents().at(10).Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue);
    APSARA_TEST_EQUAL(8, eventGroup.GetEvents().at(11).Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue);
    // APSARA_TEST_EQUAL(8, eventGroup.GetEvents().at(12).Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue);
    APSARA_TEST_EQUAL(15, eventGroup.GetEvents().at(12).Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue);
    // scrape_state
    APSARA_TEST_EQUAL(1, eventGroup.GetEvents().at(13).Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue);
    APSARA_TEST_EQUAL("OK", eventGroup.GetEvents().at(13).Cast<MetricEvent>().GetTag("status"));
    // up
    APSARA_TEST_EQUAL(1, eventGroup.GetEvents().at(14).Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue);
    APSARA_TEST_EQUAL("localhost:8080", eventGroup.GetEvents().at(14).Cast<MetricEvent>().GetTag("instance"));
    APSARA_TEST_EQUAL("test_job", eventGroup.GetEvents().at(14).Cast<MetricEvent>().GetTag("job"));
    APSARA_TEST_EQUAL("123", eventGroup.GetEvents().at(14).Cast<MetricEvent>().GetTag("lc_target_hash"));
}

void ProcessorPromRelabelMetricNativeUnittest::TestHonorLabels() {
    // make config
    Json::Value config;

    ProcessorPromRelabelMetricNative processor;
    processor.SetContext(mContext);

    string configStr;
    string errorMsg;
    configStr = configStr + R"JSON(
        {
            "job_name": "test_job",
            "scrape_timeout": "15s",
            "honor_labels": true
        }
    )JSON";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, config, errorMsg));

    // init
    APSARA_TEST_TRUE(processor.Init(config));

    // make events
    auto parser = TextParser();
    string rawData = R"""(
# begin
test_metric1{k1="v1", k2="v2"} 1.0
  test_metric2{k1="v1", k2="v2"} 2.0 1234567890
test_metric3{k1="v1",k2="v2"} 9.9410452992e+10
  test_metric4{k1="v1",k2="v2"} 9.9410452992e+10 1715829785083
  test_metric5{k1="v1", k2="v2" } 9.9410452992e+10 1715829785083
test_metric6{k1="v1",k2="v2",} 9.9410452992e+10 1715829785083
test_metric7{k1="v1",k3="2", } 9.9410452992e+10 1715829785083  
test_metric8{k1="v1", k3="v2", } 9.9410452992e+10 1715829785083
# end
    )""";
    auto eventGroup = parser.Parse(rawData, 0, 0);

    // set global labels
    eventGroup.SetTag(string("k3"), string("v3"));
    APSARA_TEST_EQUAL((size_t)8, eventGroup.GetEvents().size());
    auto targetTags = eventGroup.GetTags();
    // honor_labels is true
    processor.ProcessEvent(eventGroup.MutableEvents()[0], targetTags);
    APSARA_TEST_EQUAL("v3", eventGroup.GetEvents().at(0).Cast<MetricEvent>().GetTag(string("k3")));
    processor.ProcessEvent(eventGroup.MutableEvents()[6], targetTags);
    APSARA_TEST_EQUAL("2", eventGroup.GetEvents().at(6).Cast<MetricEvent>().GetTag(string("k3")).to_string());

    // honor_labels is false
    processor.mScrapeConfigPtr->mHonorLabels = false;
    processor.ProcessEvent(eventGroup.MutableEvents()[7], targetTags);
    APSARA_TEST_EQUAL("v3", eventGroup.GetEvents().at(7).Cast<MetricEvent>().GetTag(string("k3")).to_string());
    APSARA_TEST_EQUAL("v2", eventGroup.GetEvents().at(7).Cast<MetricEvent>().GetTag(string("exported_k3")).to_string());
}

UNIT_TEST_CASE(ProcessorPromRelabelMetricNativeUnittest, TestInit)
UNIT_TEST_CASE(ProcessorPromRelabelMetricNativeUnittest, TestProcess)
UNIT_TEST_CASE(ProcessorPromRelabelMetricNativeUnittest, TestAddAutoMetrics)
UNIT_TEST_CASE(ProcessorPromRelabelMetricNativeUnittest, TestHonorLabels)


} // namespace logtail

UNIT_TEST_MAIN

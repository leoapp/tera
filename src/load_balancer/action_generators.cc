// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>

#include <limits>

#include "glog/logging.h"
#include "load_balancer/action_generators.h"
#include "load_balancer/actions.h"
#include "load_balancer/random.h"
#include <iostream>

namespace tera {
namespace load_balancer {

RandomActionGenerator::RandomActionGenerator() :
        name_("RandomActionGenerator") {
}

RandomActionGenerator::~RandomActionGenerator() {
}

Action* RandomActionGenerator::Generate(const std::shared_ptr<Cluster>& cluster) {
    VLOG(20) << "[lb] RandomActionGenerator worked";

    if (cluster->tablet_node_num_ < 2) {
        return new EmptyAction();
    }

    uint32_t source_node_index = PickRandomNode(cluster);
    uint32_t dest_node_index = PickOtherRandomNode(cluster, source_node_index);
    uint32_t tablet_index = PickRandomTabletOfNode(cluster, source_node_index);

    if (tablet_index == kInvalidTabletIndex ||
            source_node_index == kInvalidNodeIndex ||
            dest_node_index == kInvalidNodeIndex) {
        return new EmptyAction();
    }

    return new MoveAction(tablet_index, source_node_index, dest_node_index, Name());
}

std::string RandomActionGenerator::Name() {
    return name_;
}

TabletCountActionGenerator::TabletCountActionGenerator() :
        name_("TabletCountActionGenerator") {
}

TabletCountActionGenerator::~TabletCountActionGenerator() {
}

Action* TabletCountActionGenerator::Generate(const std::shared_ptr<Cluster>& cluster) {
    VLOG(20) << "[lb] TabletCountActionGenerator worked";

    if (cluster->tablet_node_num_ < 2) {
        return new EmptyAction();
    }

    cluster->SortNodesByTabletCount();

    uint32_t source_node_index = PickMostTabletsNode(cluster);
    uint32_t dest_node_index = PickLeastTabletsNode(cluster);
    uint32_t tablet_index = PickRandomTabletOfNode(cluster, source_node_index);

    if (tablet_index == kInvalidTabletIndex ||
            source_node_index == kInvalidNodeIndex ||
            dest_node_index == kInvalidNodeIndex ||
            source_node_index == dest_node_index) {
        return new EmptyAction();
    }

    return new MoveAction(tablet_index, source_node_index, dest_node_index, Name());
}

uint32_t TabletCountActionGenerator::PickMostTabletsNode(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_tablet_count_.size() >= 1) {
        return cluster->node_index_sorted_by_tablet_count_[cluster->node_index_sorted_by_tablet_count_.size() - 1];
    } else {
        return kInvalidNodeIndex;
    }
}

uint32_t TabletCountActionGenerator::PickLeastTabletsNode(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_tablet_count_.size() >= 1) {
        uint32_t index = 0;
        if (cluster->lb_options_.meta_table_isolate_enabled) {
            while (cluster->node_index_sorted_by_tablet_count_[index] == cluster->meta_table_node_index_) {
                ++index;
                if (index == cluster->node_index_sorted_by_tablet_count_.size()) {
                    return kInvalidNodeIndex;
                }
            }
        }
        return cluster->node_index_sorted_by_tablet_count_[index];
    } else {
        return kInvalidNodeIndex;
    }
}

std::string TabletCountActionGenerator::Name() {
    return name_;
}

SizeActionGenerator::SizeActionGenerator() :
        name_("SizeActionGenerator") {
}

SizeActionGenerator::~SizeActionGenerator() {
}

Action* SizeActionGenerator::Generate(const std::shared_ptr<Cluster>& cluster) {
    VLOG(20) << "[lb] SizeActionGenerator worked";

    if (cluster->tablet_node_num_ < 2) {
        return new EmptyAction();
    }

    cluster->SortNodesBySize();

    uint32_t source_node_index = PickLargestSizeNode(cluster);
    uint32_t dest_node_index = PickSmallestSizeNode(cluster);
    uint32_t tablet_index = PickRandomTabletOfNode(cluster, source_node_index);

    if (tablet_index == kInvalidTabletIndex ||
            source_node_index == kInvalidNodeIndex ||
            dest_node_index == kInvalidNodeIndex ||
            source_node_index == dest_node_index) {
        return new EmptyAction();
    }

    return new MoveAction(tablet_index, source_node_index, dest_node_index, Name());
}

uint32_t SizeActionGenerator::PickLargestSizeNode(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_size_.size() >= 1) {
        return cluster->node_index_sorted_by_size_[cluster->node_index_sorted_by_size_.size() - 1];
    } else {
        return kInvalidNodeIndex;
    }
}

uint32_t SizeActionGenerator::PickSmallestSizeNode(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_size_.size() >= 1) {
        uint32_t index = 0;
        if (cluster->lb_options_.meta_table_isolate_enabled) {
            while (cluster->node_index_sorted_by_size_[index] == cluster->meta_table_node_index_) {
                ++index;
                if (index == cluster->node_index_sorted_by_size_.size()) {
                    return kInvalidNodeIndex;
                }
            }
        }
        return cluster->node_index_sorted_by_size_[index];
    } else {
        return kInvalidNodeIndex;
    }
}

std::string SizeActionGenerator::Name() {
    return name_;
}

ReadLoadActionGenerator::ReadLoadActionGenerator() :
        name_("ReadLoadActionGenerator") {
}

ReadLoadActionGenerator::~ReadLoadActionGenerator() {
}

Action* ReadLoadActionGenerator::Generate(const std::shared_ptr<Cluster>& cluster) {
    VLOG(20) << "[lb] ReadLoadActionGenerator worked";

    if (cluster->tablet_node_num_ < 2) {
        return new EmptyAction();
    }

    cluster->SortNodesByReadLoad();

    uint32_t source_node_index = PickMostReadNodeWithPending(cluster);
    uint32_t dest_node_index = PickLeastReadNode(cluster);
    uint32_t tablet_index = PickRandomTabletOfNode(cluster, source_node_index);

    if (tablet_index == kInvalidTabletIndex ||
            source_node_index == kInvalidNodeIndex ||
            dest_node_index == kInvalidNodeIndex ||
            source_node_index == dest_node_index) {
        return new EmptyAction();
    }

    return new MoveAction(tablet_index, source_node_index, dest_node_index, Name());
}

uint32_t ReadLoadActionGenerator::PickMostReadNode(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_read_load_.size() >= 1) {
        return cluster->node_index_sorted_by_read_load_[cluster->node_index_sorted_by_read_load_.size() - 1];
    } else {
        return kInvalidNodeIndex;
    }
}

uint32_t ReadLoadActionGenerator::PickMostReadNodeWithPending(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_read_load_.size() >= 1) {
        int i = cluster->node_index_sorted_by_read_load_.size() - 1;
        // skip node without pending
        while (i >= 0) {
            if (cluster->read_pending_nodes_index_.empty() ||
                     cluster->read_pending_nodes_index_.find(cluster->node_index_sorted_by_read_load_[i])
                     == cluster->read_pending_nodes_index_.end()) {
                --i;
            } else {
                break;
            }
        }
        if (i >= 0) {
            return cluster->node_index_sorted_by_read_load_[i];
        } else {
            return kInvalidNodeIndex;
        }
    } else {
        return kInvalidNodeIndex;
    }
}

uint32_t ReadLoadActionGenerator::PickLeastReadNode(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_read_load_.size() >= 1) {
        size_t i = 0;
        while (i < cluster->node_index_sorted_by_read_load_.size()) {
            // skip meta table node and pending node
            if ((cluster->lb_options_.meta_table_isolate_enabled && cluster->node_index_sorted_by_read_load_[i] == cluster->meta_table_node_index_) ||
                    (!cluster->read_pending_nodes_index_.empty() && cluster->read_pending_nodes_index_.find(cluster->node_index_sorted_by_read_load_[i]) != cluster->read_pending_nodes_index_.end())) {
                    ++i;
            } else {
                break;
            }
        }
        if (i < cluster->node_index_sorted_by_read_load_.size()) {
            return cluster->node_index_sorted_by_read_load_[i];
        } else {
            return kInvalidNodeIndex;
        }
    } else {
        return kInvalidNodeIndex;
    }
}

std::string ReadLoadActionGenerator::Name() {
    return name_;
}

WriteLoadActionGenerator::WriteLoadActionGenerator() :
        name_("WriteLoadActionGenerator") {
}

WriteLoadActionGenerator::~WriteLoadActionGenerator() {
}

Action* WriteLoadActionGenerator::Generate(const std::shared_ptr<Cluster>& cluster) {
    VLOG(20) << "[lb] WriteLoadActionGenerator worked";

    if (cluster->tablet_node_num_ < 2) {
        return new EmptyAction();
    }

    cluster->SortNodesByWriteLoad();

    uint32_t source_node_index = PickMostWriteNodeWithPending(cluster);
    uint32_t dest_node_index = PickLeastWriteNode(cluster);
    uint32_t tablet_index = PickRandomTabletOfNode(cluster, source_node_index);

    if (tablet_index == kInvalidTabletIndex ||
            source_node_index == kInvalidNodeIndex ||
            dest_node_index == kInvalidNodeIndex ||
            source_node_index == dest_node_index) {
        return new EmptyAction();
    }

    return new MoveAction(tablet_index, source_node_index, dest_node_index, Name());
}

uint32_t WriteLoadActionGenerator::PickMostWriteNode(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_write_load_.size() >= 1) {
        return cluster->node_index_sorted_by_write_load_[cluster->node_index_sorted_by_write_load_.size() - 1];
    } else {
        return kInvalidNodeIndex;
    }
}

uint32_t WriteLoadActionGenerator::PickMostWriteNodeWithPending(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_write_load_.size() >= 1) {
        int i = cluster->node_index_sorted_by_write_load_.size() - 1;
        // skip node without pending
        while (i >= 0) {
            if (cluster->write_pending_nodes_index_.empty() ||
                     cluster->write_pending_nodes_index_.find(cluster->node_index_sorted_by_write_load_[i])
                     == cluster->write_pending_nodes_index_.end()) {
                --i;
            } else {
                break;
            }
        }
        if (i >= 0) {
            return cluster->node_index_sorted_by_write_load_[i];
        } else {
            return kInvalidNodeIndex;
        }
    } else {
        return kInvalidNodeIndex;
    }
}

uint32_t WriteLoadActionGenerator::PickLeastWriteNode(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_write_load_.size() >= 1) {
        size_t i = 0;
        while (i < cluster->node_index_sorted_by_write_load_.size()) {
            // skip meta table node and pending node
            if ((cluster->lb_options_.meta_table_isolate_enabled && cluster->node_index_sorted_by_write_load_[i] == cluster->meta_table_node_index_) ||
                    (!cluster->write_pending_nodes_index_.empty() && cluster->write_pending_nodes_index_.find(cluster->node_index_sorted_by_write_load_[i]) != cluster->write_pending_nodes_index_.end())) {
                    ++i;
            } else {
                break;
            }
        }
        if (i < cluster->node_index_sorted_by_write_load_.size()) {
            return cluster->node_index_sorted_by_write_load_[i];
        } else {
            return kInvalidNodeIndex;
        }
    } else {
        return kInvalidNodeIndex;
    }
}

std::string WriteLoadActionGenerator::Name() {
    return name_;
}

ScanLoadActionGenerator::ScanLoadActionGenerator() :
        name_("ScanLoadActionGenerator") {
}

ScanLoadActionGenerator::~ScanLoadActionGenerator() {
}

Action* ScanLoadActionGenerator::Generate(const std::shared_ptr<Cluster>& cluster) {
    VLOG(20) << "[lb] ScanLoadActionGenerator worked";

    if (cluster->tablet_node_num_ < 2) {
        return new EmptyAction();
    }

    cluster->SortNodesByScanLoad();

    uint32_t source_node_index = PickMostScanNodeWithPending(cluster);
    uint32_t dest_node_index = PickLeastScanNode(cluster);
    uint32_t tablet_index = PickRandomTabletOfNode(cluster, source_node_index);

    if (tablet_index == kInvalidTabletIndex ||
            source_node_index == kInvalidNodeIndex ||
            dest_node_index == kInvalidNodeIndex ||
            source_node_index == dest_node_index) {
        return new EmptyAction();
    }

    return new MoveAction(tablet_index, source_node_index, dest_node_index, Name());
}

uint32_t ScanLoadActionGenerator::PickMostScanNode(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_scan_load_.size() >= 1) {
        return cluster->node_index_sorted_by_scan_load_[cluster->node_index_sorted_by_scan_load_.size() - 1];
    } else {
        return kInvalidNodeIndex;
    }
}

uint32_t ScanLoadActionGenerator::PickMostScanNodeWithPending(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_scan_load_.size() >= 1) {
        int i = cluster->node_index_sorted_by_scan_load_.size() - 1;
        // skip node without pending
        while (i >= 0) {
            if (cluster->scan_pending_nodes_index_.empty() ||
                     cluster->scan_pending_nodes_index_.find(cluster->node_index_sorted_by_scan_load_[i])
                     == cluster->scan_pending_nodes_index_.end()) {
                --i;
            } else {
                break;
            }
        }
        if (i >= 0) {
            return cluster->node_index_sorted_by_scan_load_[i];
        } else {
            return kInvalidNodeIndex;
        }
    } else {
        return kInvalidNodeIndex;
    }
}

uint32_t ScanLoadActionGenerator::PickLeastScanNode(const std::shared_ptr<Cluster>& cluster) {
    if (cluster->node_index_sorted_by_scan_load_.size() >= 1) {
        size_t i = 0;
        while (i < cluster->node_index_sorted_by_scan_load_.size()) {
            // skip meta table node and pending node
            if ((cluster->lb_options_.meta_table_isolate_enabled && cluster->node_index_sorted_by_scan_load_[i] == cluster->meta_table_node_index_) ||
                    (!cluster->scan_pending_nodes_index_.empty() && cluster->scan_pending_nodes_index_.find(cluster->node_index_sorted_by_scan_load_[i]) != cluster->scan_pending_nodes_index_.end())) {
                    ++i;
            } else {
                break;
            }
        }
        if (i < cluster->node_index_sorted_by_scan_load_.size()) {
            return cluster->node_index_sorted_by_scan_load_[i];
        } else {
            return kInvalidNodeIndex;
        }
    } else {
        return kInvalidNodeIndex;
    }
}

std::string ScanLoadActionGenerator::Name() {
    return name_;
}

} // namespace load_balancer
} // namespace tera

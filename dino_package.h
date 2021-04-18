#include <txmempool.h>
#include <validation.h>
#include <miner.h>
int find_tx_pos(std::vector<uint256> dino_vtx, uint256 tx_hash)
{
    for (std::vector<uint256>::iterator it = dino_vtx.begin(); it != dino_vtx.end(); it++) {
        if ((*it) == tx_hash)
            return it - dino_vtx.begin();
    }
    return -1;
}
bool dino_SkipMapTxEntry(CTxMemPool::txiter it, indexed_modified_transaction_set& mapModifiedTx, CTxMemPool::setEntries& failedTx,CTxMemPool::setEntries& inBlock)
{
    assert(it != mempool.mapTx.end());
    return mapModifiedTx.count(it) || inBlock.count(it) || failedTx.count(it);
}
void dino_onlyUnconfirmed(CTxMemPool::setEntries& testSet, CTxMemPool::setEntries& inBlock)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end();) {
        // Only test txs not already in the block
        if (inBlock.count(*iit)) {
            testSet.erase(iit++);
        } else {
            iit++;
        }
    }
}
bool dino_TestPackageTransactions(const CTxMemPool::setEntries& package, std::vector<uint256> dino_vtx)
{
    for (CTxMemPool::txiter it : package) {
        if (find_tx_pos(dino_vtx, it->GetTx().GetHash())>=0)
            return true;
    }
    return false;
}
void dino_SortForBlock(const CTxMemPool::setEntries& package, std::vector<CTxMemPool::txiter>& sortedEntries)
{
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}
int dino_UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded,indexed_modified_transaction_set& mapModifiedTx)
{
    int nDescendantsUpdated = 0;
    for (CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(it, descendants);
        // Insert all descendants (not yet in block) into the modified set
        for (CTxMemPool::txiter desc : descendants) {
            if (alreadyAdded.count(desc))
                continue;
            ++nDescendantsUpdated;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
                CTxMemPoolModifiedEntry modEntry(desc);
                modEntry.nSizeWithAncestors -= it->GetTxSize();
                modEntry.nModFeesWithAncestors -= it->GetModifiedFee();
                modEntry.nSigOpCostWithAncestors -= it->GetSigOpCost();
                mapModifiedTx.insert(modEntry);
            } else {
                mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
            }
        }
    }
    return nDescendantsUpdated;
}
std::vector<CTransactionRef> dino_package_tx(int& nPackagesSelected, int& nDescendantsUpdated, std::vector<uint256> dino_vtx)
{
    indexed_modified_transaction_set mapModifiedTx;
    CTxMemPool::setEntries failedTx;
    CTxMemPool::indexed_transaction_set::index<ancestor_score>::type::iterator mi = mempool.mapTx.get<ancestor_score>().begin();
    CTxMemPool::txiter iter;
    CTxMemPool::setEntries inBlock;
    std::vector<CTransactionRef> temp_block_vtx;

    while (mi != mempool.mapTx.get<ancestor_score>().end() || !mapModifiedTx.empty()) {
       
		if (mi != mempool.mapTx.get<ancestor_score>().end() &&
            dino_SkipMapTxEntry(mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx,inBlock)) {
            ++mi;
            continue;
        }

        bool fUsingModified = false;

        modtxscoreiter modit = mapModifiedTx.get<ancestor_score>().begin();
        if (mi == mempool.mapTx.get<ancestor_score>().end()) {
            // We're out of entries in mapTx; use the entry from mapModifiedTx
            iter = modit->iter;
            fUsingModified = true;
        } else {
            // Try to compare the mapTx entry to the mapModifiedTx entry
            iter = mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score>().end() &&
                CompareTxMemPoolEntryByAncestorFee()(*modit, CTxMemPoolModifiedEntry(iter))) {
                // The best entry in mapModifiedTx has higher score
                // than the one from mapTx.
                // Switch which transaction (package) to consider
                iter = modit->iter;
                fUsingModified = true;
            } else {
                // Either no entry in mapModifiedTx, or it's worse than mapTx.
                // Increment mi for the next loop iteration.
                ++mi;
            }
        }

        // We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
        // contain anything that is inBlock.
        assert(!inBlock.count(iter));

        CTxMemPool::setEntries ancestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);

        dino_onlyUnconfirmed(ancestors,inBlock);
        ancestors.insert(iter);

        // Test if all tx's are Final
        if (!dino_TestPackageTransactions(ancestors,dino_vtx)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            continue;
        }

        // Package can be added. Sort the entries in a valid order.
        std::vector<CTxMemPool::txiter> sortedEntries;
        dino_SortForBlock(ancestors, sortedEntries);
		for (size_t i = 0; i < sortedEntries.size(); ++i)
        {
            temp_block_vtx.emplace_back(sortedEntries[i]->GetSharedTx());
            inBlock.insert(sortedEntries[i]);
            mapModifiedTx.erase(sortedEntries[i]);
        }

        ++nPackagesSelected;

        // Update transactions that depend on each of these
        nDescendantsUpdated += dino_UpdatePackagesForAdded(ancestors, mapModifiedTx);
    }
    return temp_block_vtx;
}
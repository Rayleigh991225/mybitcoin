#pragma once
#include <dino_package.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <string>
#include <txmempool.h>
#include <validation.h>
struct lcs_struct {
    int value;
    int length = 1;
    int pre = -1;
};
class dino_inv
{
public:
    bool recive_full = false;
    CInv inv;
    dino_inv(CInv inv_temp)
    {
        inv = inv_temp;
        recive_full = false;
    }
};

class dino_receive_vector
{
public:
    std::vector<dino_inv> tx_vector;
    CInv dino_rrecieve;
    CInv dino_last_rrecieve;
    void add_tx(CInv inv)
    {
        bool find_tx = false;
        for (dino_inv& v : tx_vector) {
            if (inv.hash.ToString() == v.inv.hash.ToString()) {
                find_tx = true;
                break;
            }
        }
        if (!find_tx) {
            tx_vector.push_back(dino_inv(inv));
            FILE* fp;
            fp = fopen("/home/dino/Desktop/receive.txt", "a");
            fprintf(fp, "success receive a tx inv from dino node:%s \n", inv.hash.ToString().c_str());
            fclose(fp);
        } else {
            FILE* fp;
            fp = fopen("/home/dino/Desktop/receive.txt", "a");
            fprintf(fp, "faild receive a tx inv from dino node:%s \n", inv.hash.ToString().c_str());
            fclose(fp);
        }
    }
    void update_receive()
    {
        for (dino_inv& v : tx_vector) {
            if ((mempool.exists(v.inv.hash)) || (v.recive_full == true)) {
                dino_rrecieve = v.inv;
                if (v.recive_full == false) {
                    v.recive_full = true;
                    FILE* fp;
                    fp = fopen("/home/dino/Desktop/receive.txt", "a");
                    fprintf(fp, "success receive a full tx inv from dino node:%s \n", v.inv.hash.ToString().c_str());
                    fclose(fp);
                }

            } else
                break;
        }
    }
    void add_full_tx(uint256 tx_hash)
    {
        for (dino_inv& v : tx_vector) {
            if (v.inv.hash == tx_hash) {
                if (v.recive_full == false) {
                    v.recive_full = true;
                    FILE* fp;
                    fp = fopen("/home/dino/Desktop/receive.txt", "a");
                    fprintf(fp, "success receive a full tx inv from dino node:%s \n", v.inv.hash.ToString().c_str());
                    fclose(fp);
                }
                break;
            }
        }
    }
    int find_tx(uint256 tx_hash)
    {
        for (std::vector<dino_inv>::iterator it = tx_vector.begin(); it != tx_vector.end(); it++) {
            if (it->inv.hash == tx_hash) {
                return it - tx_vector.begin();
            }
        }
        return -1;
    }
};

class dino_send_vector
{
public:
    std::vector<dino_inv> tx_vector;
    CInv dino_srecieve;
    void add_tx(CInv inv)
    {
        bool find_tx = false;
        for (dino_inv& v : tx_vector) {
            if (inv.hash == v.inv.hash) {
                find_tx = true;
                break;
            }
        }
        if (!find_tx) {
            tx_vector.push_back(dino_inv(inv));
            FILE* fp;
            fp = fopen("/home/dino/Desktop/send.txt", "a");
            fprintf(fp, "success send a tx inv from dino node:%s \n", inv.hash.ToString().c_str());
            fclose(fp);
        } else {
            FILE* fp;
            fp = fopen("/home/dino/Desktop/send.txt", "a");
            fprintf(fp, "faild send a tx inv from dino node:%s \n", inv.hash.ToString().c_str());
            fclose(fp);
        }
    }

    int find_tx(uint256 tx_hash)
    {
        for (std::vector<dino_inv>::iterator it = tx_vector.begin(); it != tx_vector.end(); it++) {
            if (it->inv.hash == tx_hash) {
                return it - tx_vector.begin();
            }
        }
        return -1;
    }
};

class Dino_block
{
public:
    CBlockHeader dino_header;
    CTransactionRef coinbase_transaction;
    std::vector<CTransactionRef> lose_transaction;
    int sreceive_max;
    int rreceive_max;
    std::set<int> del_transaction;
    std::map<int, int> reorder_transaction;
    Dino_block()
    {
        this->sreceive_max = -1;
        this->rreceive_max = -1;
    }
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITEAS(CBlockHeader, this->dino_header);
        READWRITE(coinbase_transaction);
        READWRITE(lose_transaction);
        READWRITE(sreceive_max);
        READWRITE(rreceive_max);
        READWRITE(del_transaction);
        READWRITE(reorder_transaction);
    }
};

std::set<int> get_lcs(std::map<uint256, int> vtx, std::vector<CTransactionRef> block_vtx)
{
    lcs_struct a[10000];
    int temp = 0;
    for (CTransactionRef& v : block_vtx) {
        if (vtx.count(v->GetHash())) {
            a[temp].value = vtx[v->GetHash()];
            temp = temp + 1;
        }
    }

    int pos = 0;
    for (int i = 0; i < temp; i++) {
        for (int j = 0; j < i; j++) {
            if ((a[j].value < a[i].value) && (a[j].length + 1 >= a[i].length)) {
                a[i].pre = j;
                a[i].length = a[j].length + 1;
            }
        }
        if (a[i].length >= a[pos].length) {
            pos = i;
        }
    }
    std::set<int> temp_lcs;
    while (pos != -1) {
        temp_lcs.insert(a[pos].value);
        pos = a[pos].pre;
    }
    return temp_lcs;
}
std::map<uint256, int> get_map_block_vtx(std::vector<CTransactionRef> vtx)
{
    std::map<uint256, int> temp_map;
    int temp = 0;
    for (CTransactionRef& v : vtx) {
        if (v->IsCoinBase())
            continue;
        temp_map[v->GetHash()] = temp;
        temp = temp + 1;
    }
    return temp_map;
}
int get_max_send_index(const std::shared_ptr<const CBlock>& pblock, dino_send_vector send_dino)
{
    int find_result = -1;
    int max_result = -1;
    int sreceive_max = send_dino.find_tx(send_dino.dino_srecieve.hash);
    for (std::vector<CTransactionRef>::const_iterator it = pblock->vtx.begin(); it != pblock->vtx.end(); it++) {
        if ((*it)->IsCoinBase())
            continue;
        find_result = send_dino.find_tx((*it)->GetHash());
        if ((find_result > max_result) && (find_result <= sreceive_max))
            max_result = find_result;
    }
    FILE* fp;
    fp = fopen("/home/dino/Desktop/sr_max.txt", "a");
    fprintf(fp, "we success get the dino send max_index:%d %d\n", sreceive_max, max_result);
    fclose(fp);
    return max_result;
}
int get_max_receive_index(const std::shared_ptr<const CBlock>& pblock, dino_receive_vector receive_dino)
{
    FILE* fp;
    int find_result = -1;
    int max_result = -1;
    int rreceive_max = receive_dino.find_tx(receive_dino.dino_rrecieve.hash);
    for (std::vector<CTransactionRef>::const_iterator it = pblock->vtx.begin(); it != pblock->vtx.end(); it++) {
        if ((*it)->IsCoinBase())
            continue;
        find_result = receive_dino.find_tx((*it)->GetHash());
        if ((find_result > max_result) && (find_result <= rreceive_max))
            max_result = find_result;
    }
    fp = fopen("/home/dino/Desktop/sr_max.txt", "a");
    fprintf(fp, "we success get the dino receive max_index:%d %d\n", rreceive_max, max_result);
    fclose(fp);
    return max_result;
}
std::vector<uint256> get_dino_vtx(int rreceive_max, dino_receive_vector receive_dino, int sreceive_max, dino_send_vector send_dino)
{
    std::vector<uint256> temp_vector;
    for (std::vector<dino_inv>::iterator it = receive_dino.tx_vector.begin(); it != receive_dino.tx_vector.end(); it++) {
        if ((it - receive_dino.tx_vector.begin()) <= rreceive_max) {
            temp_vector.emplace_back(it->inv.hash);
        } else
            break;
    }
    for (std::vector<dino_inv>::iterator it = send_dino.tx_vector.begin(); it != send_dino.tx_vector.end(); it++) {
        if ((it - send_dino.tx_vector.begin()) <= sreceive_max) {
            temp_vector.emplace_back(it->inv.hash);
        } else
            break;
    }
    return temp_vector;
}
std::vector<CTransactionRef> get_lose_transaction(const std::shared_ptr<const CBlock>& pblock, std::vector<uint256> dino_vtx)
{
    std::vector<CTransactionRef> temp_lose_transaction;
    for (std::vector<CTransactionRef>::const_iterator it = pblock->vtx.begin(); it != pblock->vtx.end(); it++) {
        if ((*it)->IsCoinBase())
            continue;
        if (find_tx_pos(dino_vtx, (*it)->GetHash()) == -1) {
            temp_lose_transaction.emplace_back(*it);
        }
    }
    return temp_lose_transaction;
}
void get_del_reorder_transaction(const std::shared_ptr<const CBlock>& pblock, std::vector<CTransactionRef> block_vtx, Dino_block* dino_block)
{
    std::set<int> lcs;
    std::map<uint256, int> prediction_block_map;
    std::map<uint256, int> pblock_map;
    prediction_block_map = get_map_block_vtx(block_vtx);
    pblock_map = get_map_block_vtx(pblock->vtx);

    lcs = get_lcs(prediction_block_map, pblock->vtx);
    int temp = 0;
    for (CTransactionRef& v : block_vtx) {
        if (v->IsCoinBase())
            continue;
        if (!lcs.count(temp)) {
            if (!pblock_map.count(v->GetHash())) {
                dino_block->del_transaction.insert(temp);
            } else {
                dino_block->reorder_transaction[temp] = pblock_map[v->GetHash()];
            }
        }
        temp = temp + 1;
    }
}
Dino_block create_dinoblock(const std::shared_ptr<const CBlock>& pblock, dino_send_vector send_dino, dino_receive_vector receive_dino)
{
    FILE* fp;
    FILE* fp1;
    FILE* fp2;
    int we_not_have = 0;
    fp1 = fopen("/home/dino/Desktop/origin_block_vtx_lose.txt", "a");
    fp = fopen("/home/dino/Desktop/origin_block_vtx.txt", "a");
    fp2 = fopen("/home/dino/Desktop/origin_block_vtx_in_sr.txt", "a");

    fprintf(fp, "block hash is:%s block vtx size=%d\n", pblock->GetHash().ToString().c_str(), pblock->vtx.size());
    fprintf(fp1, "block hash is:%s block vtx size=%d\n", pblock->GetHash().ToString().c_str(), pblock->vtx.size());
    fprintf(fp2, "block hash is:%s block vtx size=%d\n", pblock->GetHash().ToString().c_str(), pblock->vtx.size());
    for (std::vector<CTransactionRef>::const_iterator it = pblock->vtx.begin(); it != pblock->vtx.end(); it++) {
        if (mempool.exists((*it)->GetHash()))
            fprintf(fp, "have origin tx:%s\n", (*it)->GetHash().ToString().c_str());
        else {
            fprintf(fp, "not have origin tx:%s\n", (*it)->GetHash().ToString().c_str());
            fprintf(fp1, "not have origin tx:%s\n", (*it)->GetHash().ToString().c_str());
            we_not_have++;
        }
        if ((send_dino.find_tx((*it)->GetHash()) == -1) && (receive_dino.find_tx((*it)->GetHash())==-1)) {
            fprintf(fp2, "sr not have origin tx:%s\n", (*it)->GetHash().ToString().c_str());
        } else {
            if ((send_dino.find_tx((*it)->GetHash()) == -1)) {
                fprintf(fp2, "r have tx:%s position=%d\n", (*it)->GetHash().ToString().c_str(), receive_dino.find_tx((*it)->GetHash()));
			}
			else
			{
                fprintf(fp2, "s have tx:%s position=%d\n", (*it)->GetHash().ToString().c_str(), send_dino.find_tx((*it)->GetHash()));
			}
        }
    }
    fprintf(fp1, "block hash is:%s block vtx we lose size=%d\n", pblock->GetHash().ToString().c_str(), we_not_have);
    fclose(fp);
    fclose(fp1);
    fclose(fp2);

    Dino_block dino_block = Dino_block();
    dino_block.dino_header = pblock->GetBlockHeader();
    dino_block.coinbase_transaction = pblock->vtx[0];

    std::vector<uint256> dino_vtx;
    std::vector<CTransactionRef> block_vtx;

    dino_block.rreceive_max = get_max_send_index(pblock, send_dino);
    dino_block.sreceive_max = get_max_receive_index(pblock, receive_dino);


    dino_vtx = get_dino_vtx(dino_block.sreceive_max, receive_dino, dino_block.rreceive_max, send_dino);

    /*fp = fopen("/home/dino/Desktop/dino_block_vtx_srmax.txt", "a");
    fprintf(fp, "block hash is:%s\n", pblock->GetHash().ToString().c_str());
    for (uint256& v : dino_vtx) {
        fprintf(fp, "%s\n", v.ToString().c_str());
    }
    fclose(fp);*/

    dino_block.lose_transaction = get_lose_transaction(pblock, dino_vtx);

    fp = fopen("/home/dino/Desktop/dino_lose_transaction.txt", "a");
    fprintf(fp, "block hash is:%s\n lose size=%d all size= %d\n ", pblock->GetHash().ToString().c_str(), dino_block.lose_transaction.size(), pblock->vtx.size());
    for (std::vector<CTransactionRef>::const_iterator it = dino_block.lose_transaction.begin(); it != dino_block.lose_transaction.end(); it++) {
        fprintf(fp, "lose transaction:%s\n", (*it)->GetHash().ToString().c_str());
    }
    fclose(fp);

    for (CTransactionRef& v : dino_block.lose_transaction) {
        dino_vtx.emplace_back(v->GetHash());
    }

    /*fp = fopen("/home/dino/Desktop/dino_block_vtx_lose.txt", "a");
    fprintf(fp, "block hash is:%s\n", pblock->GetHash().ToString().c_str());
    for (uint256& v : dino_vtx) {
        fprintf(fp, "%s\n", v.ToString().c_str());
    }
    fclose(fp);*/
   
        fp = fopen("/home/dino/Desktop/dino_block_try_add.txt", "a");
        fprintf(fp, "block hash is:%s\n", pblock->GetHash().ToString().c_str());
        for (CTransactionRef& v : dino_block.lose_transaction) {
            CValidationState state;
            bool fMissingInputs = false;
            std::list<CTransactionRef> lRemovedTxn;
            if (AcceptToMemoryPool(mempool, state, v, &fMissingInputs, &lRemovedTxn, false /* bypass_limits */, 0 /* nAbsurdFee */)) {
                fprintf(fp, "success add:%s\n", v->GetHash().ToString().c_str());

            } else {
                fprintf(fp, "failed add:%s\n", v->GetHash().ToString().c_str());
            }
            receive_dino.add_full_tx(v->GetHash());
        }
        fclose(fp);

        receive_dino.update_receive();
   
    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;
    block_vtx = dino_package_tx(nPackagesSelected, nDescendantsUpdated, dino_vtx);

    /*fp = fopen("/home/dino/Desktop/dino_block_predict.txt", "a");
    fprintf(fp, "block hash is:%s\n", pblock->GetHash().ToString().c_str());
    for (std::vector<CTransactionRef>::const_iterator it = block_vtx.begin(); it != block_vtx.end(); it++) {
        fprintf(fp, "%s\n", (*it)->GetHash().ToString().c_str());
    }
    fclose(fp);*/

    get_del_reorder_transaction(pblock, block_vtx, &dino_block);

    /*fp = fopen("/home/dino/Desktop/dino_del.txt", "a");
    fprintf(fp, "block hash is:%s\n", pblock->GetHash().ToString().c_str());
    for (std::set<int>::iterator it = dino_block.del_transaction.begin(); it != dino_block.del_transaction.end(); it++) {
        fprintf(fp, "%d\n", (*it));
    }
    fclose(fp);

    fp = fopen("/home/dino/Desktop/dino_reorder.txt", "a");
    fprintf(fp, "block hash is:%s\n", pblock->GetHash().ToString().c_str());
    for (std::map<int, int>::iterator it = dino_block.reorder_transaction.begin(); it != dino_block.reorder_transaction.end(); it++) {
        fprintf(fp, "%d %d\n", (*it).first, (*it).second);
    }
    fclose(fp);*/

    return dino_block;
}
CBlock rebuild_from_dino(Dino_block dino_block, dino_receive_vector receive_dino, dino_send_vector send_dino)
{
    LOCK(cs_main);
    CBlock tblock = CBlock();
    FILE* fp;
    tblock.nVersion = dino_block.dino_header.nVersion;
    tblock.hashPrevBlock = dino_block.dino_header.hashPrevBlock;
    tblock.hashMerkleRoot = dino_block.dino_header.hashMerkleRoot;
    tblock.nBits = dino_block.dino_header.nBits;
    tblock.nNonce = dino_block.dino_header.nNonce;
    tblock.nTime = dino_block.dino_header.nTime;

    tblock.vtx.emplace_back(dino_block.coinbase_transaction);

    std::vector<CTransactionRef> block_vtx;
    std::vector<uint256> dino_vtx;

    dino_vtx = get_dino_vtx(dino_block.rreceive_max, receive_dino, dino_block.sreceive_max, send_dino); //need change

    fp = fopen("/home/dino/Desktop/dino_rebuild_block_try_add.txt", "a");
    for (CTransactionRef& v : dino_block.lose_transaction) {
        CValidationState state;
        bool fMissingInputs = false;
        std::list<CTransactionRef> lRemovedTxn;
        if (AcceptToMemoryPool(mempool, state, v, &fMissingInputs, &lRemovedTxn, false /* bypass_limits */, 0 /* nAbsurdFee */)) {
            fprintf(fp, "success add:%s\n %s\n", v->GetHash().ToString().c_str(), dino_block.dino_header.GetHash().ToString().c_str());
        } else {
            fprintf(fp, "failed add:%s\n %s\n", v->GetHash().ToString().c_str(), dino_block.dino_header.GetHash().ToString().c_str());
        }
    }
    fclose(fp);

    for (CTransactionRef& v : dino_block.lose_transaction) {
        dino_vtx.emplace_back(v->GetHash());
    }

    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;
    block_vtx = dino_package_tx(nPackagesSelected, nDescendantsUpdated, dino_vtx);

    fp = fopen("/home/dino/Desktop/dino_rebuild_block_predict.txt", "a");
    for (std::vector<CTransactionRef>::const_iterator it = block_vtx.begin(); it != block_vtx.end(); it++) {
        fprintf(fp, "%s\n %s\n", (*it)->GetHash().ToString().c_str(), dino_block.dino_header.GetHash().ToString().c_str());
    }
    fclose(fp);

    std::vector<CTransactionRef> block_temp_vtx(block_vtx.size() - dino_block.del_transaction.size(), NULL);
    int pos = 0;
    int temp = 0;
    for (std::map<int, int>::const_iterator it = dino_block.reorder_transaction.begin(); it != dino_block.reorder_transaction.end(); it++) {
        block_temp_vtx[it->second] = *(block_vtx.begin() + it->first);
    }
    for (CTransactionRef& v : block_vtx) {
        while ((pos < block_vtx.size() - dino_block.del_transaction.size()) && (block_temp_vtx[pos] != NULL)) {
            pos++;
        }
        if (dino_block.del_transaction.count(temp)) {
            temp++;
            continue;
        }
        if (!dino_block.reorder_transaction.count(temp)) {
            block_temp_vtx[pos] = v;
        }
        temp++;
    }

    for (CTransactionRef& v : block_temp_vtx) {
        tblock.vtx.emplace_back(v);
    }

    fp = fopen("/home/dino/Desktop/dino_block_rebuild_log.txt", "a");
    fprintf(fp, "we have success rebuild block hash is:%s\n", tblock.GetHash().ToString().c_str());
    fclose(fp);

    fp = fopen("/home/dino/Desktop/dino_block_rebuild.txt", "a");
    fprintf(fp, "block hash is:%s\n", tblock.GetHash().ToString().c_str());
    for (std::vector<CTransactionRef>::const_iterator it = tblock.vtx.begin(); it != tblock.vtx.end(); it++) {
        fprintf(fp, "%s\n %s\n", (*it)->GetHash().ToString().c_str(), dino_block.dino_header.GetHash().ToString().c_str());
    }
    fclose(fp);

    return tblock;
}
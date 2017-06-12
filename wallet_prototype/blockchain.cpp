#include "stdafx.h"
#include "blockchain.h"


Blockchain::Blockchain()
{
}
bool Blockchain::addBlock(Block& block)
{
	if (validateBlock(block))
	{
		blockChain.push_back(block);
		return true;
	}
	return false;
}
bool Blockchain::validateBlockChain()
{
	bool validated = true;
	for (auto i : nonValidatedBlockChain)
	{
		if (validateBlock(i))
		{
			blockChain.push_back(i);
		}
		else
		{
			blockChain.clear();
			nonValidatedBlockChain.clear();
			Transaction::availibleTxes.clear();
			validated = false;
			break;
		}
	}
	nonValidatedBlockChain.clear();
	return validated;
}
void Blockchain::setAvailibleTxes(Block& block)
{
	for (size_t i = 0; i < block.txs.size(); i++)
	{
		block.txs[i].addAvailibleTxe(Output(block.currentNumber, i), block.txs[i].tails.size());
	}
}
bool Blockchain::validateBlock(Block& block)
{
	if (bits != block.bits)
		return false;
	if (block.currentNumber != blockChain.size())
		return false;
	if (block.currentNumber == 0 && block.prevBlock.size() != 0)
	    return false;
	if (block.currentNumber != 0 && block.prevBlock != SHA_256().process(blockChain[blockChain.size() - 1].getBlockData()))
		return false;
	if (!validateMerkleRoot(block))
		return false;
	secure_vector<byte> hash = SHA_256().process(block.getBlockData());
	bool validated = true;
	for (auto i = 0; i < bits; i++)
	{
		if (hash[i] != 0)
			validated = false;
	}
	if (block.txs.size() > 0)
	{
		validated = validateFirstTxn(block.txs[0]);
	}
	else
		validated = false;
	for (auto i = 1; i < block.txs.size(); i++)
	{
		std::vector<std::pair<Output, size_t>> toRestore;
		validated =  validateTxn(block.txs[i], toRestore);
		if (!validated)
			break;
	}
	if (validated)
	{
		setAvailibleTxes(block);
		clearAvailibleTxes();
	}
		
	return validated;
}
void Blockchain::clearAvailibleTxes()
{
	for (auto i =0;i<Transaction::availibleTxes.size();i++)
	{
		bool toRemove = true;
		for (auto j : Transaction::availibleTxes[i].usedTails)
		{
			toRemove = j && toRemove;
		}
		if (toRemove)
		{
			Transaction::availibleTxes.erase(Transaction::availibleTxes.begin() + i);
		}
	}
}
bool Blockchain::validateFirstTxn(const Transaction& txn)
{
	bool validated = txn.inputs.size() == 0 && txn.pubKey.size()==0 && txn.signature.size()==0 && txn.tails.size()==1;
	if (validated)
	{
		if (txn.tails[0].getInfo().first == 100)//HARDCODED
			return true;
	}
	return false;
}
size_t Blockchain::validateInputs(const Transaction& txn, std::vector<std::pair<Output, size_t>> toRestore)
{
	secure_vector<byte> address = SHA_256().process(txn.pubKey);
	size_t sum = 0;
	for (auto i : txn.inputs)//because of first TXE use iterators;
	{
		std::pair<Output, size_t> info = i.getInfo();
		auto counter = 0;
		bool found = false;
		for (counter; counter < Transaction::availibleTxes.size(); counter++)
		{

			if (Transaction::availibleTxes[counter].output == info.first)//Output operator
			{
				if (Transaction::availibleTxes[counter].usedTails.size() > info.second && !Transaction::availibleTxes[counter].usedTails[info.second])
				{
					found = true;
					if (!(blockChain[info.first.blockNumber].txs[info.first.txeNumber].tails[info.second].getInfo().second == address))
						return 0;
					sum += blockChain[info.first.blockNumber].txs[info.first.txeNumber].tails[info.second].getInfo().first;
					toRestore.push_back(info);
					Transaction::availibleTxes[counter].usedTails[info.second] = true;
				}
				else
					return 0;
			}
		}
		if (!found)
			return 0;
	}
	return sum;
}
bool Blockchain::validateSignature(const Transaction& txn)
{
	auto signature = txn.signature;
	auto data = txn.getTxeData();
	Public_Key* thePublicKey = X509::load_key(txn.pubKey);
	PK_Verifier verifier(*thePublicKey, "EMSA1(SHA-256)");
	verifier.update(data);
	return verifier.check_signature(signature);
}
bool Blockchain::validateTails(const Transaction& txn, size_t sum)
{
	size_t sendMoney = 0;
	for (auto i : txn.tails)
	{
		sendMoney += i.getInfo().first;
	}
	if (sendMoney <= sum)
		return true;
}
void Blockchain::restore(std::vector<std::pair<Output, size_t>> toRestore)
{
	for (auto i : toRestore)
	{
		for (auto counter = 0; counter < Transaction::availibleTxes.size(); counter++)
		{

			if (Transaction::availibleTxes[counter].output == i.first)
			{
				Transaction::availibleTxes[counter].usedTails[i.second] = false;
			}
		}
	}
}

bool Blockchain::validateTxn(const Transaction& txn, std::vector<std::pair<Output, size_t>> toRestore)//remember ABout First TXE in List
{
	size_t sum;
	if (!validateSignature(txn))
		return false;
	if ((sum = validateInputs(txn, toRestore)) == 0)
	{
		restore(toRestore);
		return false;
	}
	if (!validateTails)
	{
		restore(toRestore);
		return false;
	}
	return true;

}
bool Blockchain::validateMerkleRoot(const Block& block)
{
	if (block.txs.size() == 1 || block.txs.size() == 2 || block.txs.size() == 4 || block.txs.size() == 8)//HARDCODE
	{
		std::vector<secure_vector<byte>> hashes;
		for (auto i = 0; i < block.txs.size(); i++)
		{
			hashes.push_back(SHA_256().process(block.txs[i].getTxeData()));
		}
		while (hashes.size() != 1)
		{
			std::vector<secure_vector<byte>> tempHashes;
			for (auto i = 0; i < hashes.size(); i++)
			{
				if (i % 2 == 1)
				{
					secure_vector<byte> tempData(hashes[i - 1]);
					for (auto j : hashes[i])
					{
						tempData.push_back(j);
					}
					tempHashes.push_back(SHA_256().process(tempData));
				}
				hashes = tempHashes;
			}
		}
		if (block.merkleRoot == hashes[0])
			return true;
		else
			return false;
	}
	return false;
}
Blockchain::~Blockchain()
{
}

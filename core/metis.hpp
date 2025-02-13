/*
 * @Author: chency 1013858730@qq.com
 * @Date: 2022-08-16 16:34:11
 * @LastEditors: chency 1013858730@qq.com
 * @LastEditTime: 2022-11-22 17:04:10
 * @FilePath: /NtsMinibatch/NeutronStarLite-master/core/metis.hpp
 */
#ifndef METIS_HPP
#define METIS_HPP
#include <assert.h>
#include <map>
#include <math.h>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <iterator>
#include <algorithm>
#include <metis.h>

#include "core/graph.hpp"
#include "core/coocsc.hpp"
class MetisPartition{
public:
    MetisPartition() = default;
    MetisPartition(idx_t vertices, idx_t target_parts) : vertices_(vertices), target_parts_(target_parts), objval_(0)
    { 
        balance_constraint_ = 1;
        batchTrainVertices_.resize(target_parts);
        part_.resize(vertices_);
        vweight_.resize(vertices_, 0);
    }

    // void TestPartGraph(PartitionedGraph *partitioned_graph){
    //     testVertices = partitioned_graph->owned_vertices;
    //     VertexId* column_offset = new VertexId[partitioned_graph->global_vertices+1];
    //     memcpy(column_offset, partitioned_graph->column_offset, sizeof(VertexId)*(partitioned_graph->owned_vertices+1));
    //     std::cout<<"partition id:\t "<<partitioned_graph->partition_id<<"\t\t"<<column_offset[partitioned_graph->owned_vertices]<<"\t\t"<<partitioned_graph->column_offset[partitioned_graph->owned_vertices]<<std::endl;
    //     for(int i = partitioned_graph->owned_vertices+1; i < partitioned_graph->global_vertices+1; i++){
    //         column_offset[i] = column_offset[partitioned_graph->owned_vertices];
    //     }
    //     SetCSC(column_offset, partitioned_graph->row_indices);
    // }

    void SetCSC(VertexId* column_offset, VertexId* row_indices){
        for(VertexId i = 0; i <vertices_; i++){
            xadj_.push_back(column_offset[i]);
            for(int j = column_offset[i]; j < column_offset[i+1]; j++){
                adjncy_.push_back(row_indices[j]);
            }
        }
        xadj_.push_back(column_offset[vertices_]);
    }

    void debug(){
        std::cout<<"vertices:\t"<<vertices_<<std::endl;
        std::cout<<"xadj_.data()\t"<<xadj_.size()<<std::endl;
        std::cout<<"adjncy_.data()\t"<<adjncy_.size()<<std::endl;
        std::cout<<"vweight_.data()\t"<<vweight_.size()<<std::endl;
        std::cout<<"target_parts_\t"<<target_parts_<<std::endl;
        for(int i = 0; i < adjncy_.size(); i ++){
            if(adjncy_[i]>=232965 || adjncy_[i]<0){
                std::cout<<adjncy_[i]<<"error"<<std::endl;
            }
        }
    }

    void GetKwayPartition(bool balance = true){
        // idx_t options[METIS_NOPTIONS];
        // METIS_SetDefaultOptions(options);
        // options[METIS_OPTION_NUMBERING] = 0;
        // idx_t options[METIS_NOPTIONS];
        // METIS_SetDefaultOptions(options);
        // options[METIS_OPTION_ONDISK] = 1;
        // options[METIS_OPTION_NITER] = 1;
        // options[METIS_OPTION_NIPARTS] = 1;
        // options[METIS_OPTION_DROPEDGES] = 1;
        int test = -1;
        if(!balance){
            LOG_INFO("No balance KwayPartition Method");
            test = METIS_PartGraphKway(&vertices_,&balance_constraint_, xadj_.data(), adjncy_.data(),
				       NULL, NULL, NULL, &target_parts_, NULL,
       				  NULL, NULL, &objval_, part_.data());
        }else{
            LOG_INFO("balance KwayPartition Method");
            SetTrainVerticesWeight();
            test = METIS_PartGraphKway(&vertices_,&balance_constraint_, xadj_.data(), adjncy_.data(),
                        vweight_.data(), NULL, NULL, &target_parts_, NULL,
                        NULL, NULL, &objval_, part_.data());
        }
        if(test == METIS_OK){
            std::cout<<"METIS_OK"<<std::endl;
        }else if(test == METIS_ERROR_INPUT){
            std::cout<<"METIS_ERROR_INPUT"<<std::endl;
        }else if(test == METIS_ERROR_MEMORY){
            std::cout<<"METIS_ERROR_MEMORY"<<std::endl;
        }else if(test == METIS_ERROR){
            std::cout<<"METIS_ERROR"<<std::endl;
        }
        SetBatchTrainVertices();
    }

    void GetRecursivePartition(bool balance = true){
        int test = -1;
        if(!balance){
            LOG_INFO("No balance RecursivePartition Method");
            test = METIS_PartGraphRecursive(&vertices_,&balance_constraint_, xadj_.data(), adjncy_.data(),
				       NULL, NULL, NULL, &target_parts_, NULL,
       				  NULL, NULL, &objval_, part_.data());
        }else{
            LOG_INFO("balance RecursivePartition Method");
            SetTrainVerticesWeight();
            test = METIS_PartGraphRecursive(&vertices_,&balance_constraint_, xadj_.data(), adjncy_.data(),
				       vweight_.data(), NULL, NULL, &target_parts_, NULL,
       				  NULL, NULL, &objval_, part_.data());
        }
        if(test == METIS_OK){
            std::cout<<"METIS_OK"<<std::endl;
        }else if(test == METIS_ERROR_INPUT){
            std::cout<<"METIS_ERROR_INPUT"<<std::endl;
        }else if(test == METIS_ERROR_MEMORY){
            std::cout<<"METIS_ERROR_MEMORY"<<std::endl;
        }else if(test == METIS_ERROR){
            std::cout<<"METIS_ERROR"<<std::endl;
        }
        SetBatchTrainVertices();
    }

    //sequential partition(=default partition)
    void GetDefaultPartition(int batch_size){
        batchTrainVertices_.resize((trainVerticesId_.size()-1)/batch_size + 1);
        int count = 0;
        for(int i = 0; i < trainVerticesId_.size(); i++){
            batchTrainVertices_[count].push_back(trainVerticesId_[i]);
            if(((i+1)%batch_size)==0){
                count++;
            }
        }
        // std::cout<<"train id\t\t\t"<<std::endl;
        // for(int i = 0; i < trainVerticesId_.size(); i++){
        //     std::cout<<trainVerticesId_[i]<<std::endl;
        // }
        // std::cout<<"count:\t\t\t"<<count<<std::endl;
        // for(int i = 0; i < batchTrainVertices_.size(); i++){
        //     std::cout<<"this is \t"<<i<<"\tsize:\t"<<batchTrainVertices_[i].size()<<std::endl;
        //     for(int j = 0; j < batchTrainVertices_[i].size(); j++)
        //         std::cout<<batchTrainVertices_[i][j]<<"\t"<<std::end;
        //     std::cout<<std::endl;
        // }
    }

    void GetRandomPartition(int batch_size){
        batchTrainVertices_.resize((trainVerticesId_.size()-1)/batch_size + 1);
        std::vector<int> numbers;
        for(int i = 0;i<trainVerticesId_.size();i++)
        {
            numbers.push_back(i);
        }
        srand(time(0));
        std::random_shuffle(numbers.begin(),numbers.end());
        int count = 0;
        for(int i = 0; i < trainVerticesId_.size(); i++){
            batchTrainVertices_[count].push_back(trainVerticesId_[numbers[i]]);
            if(((i+1)%batch_size)==0){
                count++;
            }
        }
    }

    void GetBatchPartition(std::string method, int batch_size){
        if(0 == method.compare("Seq")){
            LOG_INFO("Sequential Method");
            GetDefaultPartition(batch_size);
        }else if(0 ==  method.compare("MetisBalnce")){
            if(this->GetParts() > 8){ 
                this->GetKwayPartition();  
            }else{
                this->GetRecursivePartition();
            }
        }else if(0 ==  method.compare("MetisDefault")){
            if(this->GetParts() > 8){
                this->GetKwayPartition(false);  
            }else{
                this->GetRecursivePartition(false);
            }
        }else if(0 ==  method.compare("MetisRandom")){
            this->GetRandomPartition(batch_size);  
        }else{
            LOG_INFO("not supported partition method");
        }
    }

    void GetBatchPartition(std::string method, int batch_size, int type){
        //first stage : devide the vertex type of process
        if (type==0){ //train
            trainVerticesId_ = trainMaskVertex;
        }else if (type==1) //val
        {
            trainVerticesId_ = valMaskVertex;
        }else if (type==2)  //test
        { 
            trainVerticesId_ = testMaskVertex;
        }
        //second stage: get specific partition method
        if(0 == method.compare("Seq")){
            LOG_INFO("Sequential Method");
            GetDefaultPartition(batch_size);
        }else if(0 ==  method.compare("MetisBalnce")){
            if(this->GetParts() > 8){
                this->GetKwayPartition();  
            }else{
                this->GetRecursivePartition();
            }
        }else if(0 ==  method.compare("MetisDefault")){
            if(this->GetParts() > 8){
                this->GetKwayPartition(false);  
            }else{
                this->GetRecursivePartition(false);
            }
        }else if(0 ==  method.compare("MetisRandom")){
            this->GetRandomPartition(batch_size);  
        }else{
            LOG_INFO("not supported partition method");
        }
    }

    void SetBatchTrainVertices(){
        // #pragma omp parallel for  //may be can't parallel do 
        for(int i = 0; i < part_.size(); i++){
            if(!is_exist(trainVerticesId_, i)) continue;
            batchTrainVertices_[part_[i]].push_back(i);
        }
        // for(int i = 0; i < testVertices; i++){
        //     if(!is_exist(trainVerticesId_, i)) continue;
        //     batchTrainVertices_[part_[i]].push_back(i);
        // }
        // for(int i = 0; i < batchTrainVertices_.size(); i++){
        //     std::cout<<"batch train:\t"<<i<<"\tsize:\t"<<batchTrainVertices_[i].size()<<std::endl;
        //     for(int j = 0; j < batchTrainVertices_[i].size(); j++){
        //         std::cout<<batchTrainVertices_[i][j]<<std::endl;
        //     }
        //     std::cout<<std::endl;
        // }
    }

    /**
   * @description: 
   * @note:  stores the local id(id - localstartid)
   * @param {int*} local_mask
   * @param {int} num
   * @return {*}
   */
    void SetMaskVertex(int* local_mask, int num){  
        for(int i = 0; i < num; i++)
        {
        if (local_mask[i] == 0) {
            trainMaskVertex.push_back(i);
        } else if (local_mask[i] == 1) {
            valMaskVertex.push_back(i);
        } else if (local_mask[i] == 2) {
            testMaskVertex.push_back(i);
        }
        }
    }

    void SetTrainVertices(std::vector<uint32_t> & trainVerticesId){
        trainVerticesId_ = trainVerticesId;
    }

    void SetTrainVerticesWeight(){
        #pragma omp parallel for
        for(int i = 0; i < vertices_; i++){
            if(is_exist(trainVerticesId_, i)){
                vweight_[i] = 1;
            }
        }
    }

    int GetTrainVerticesCount(){
        return trainVerticesId_.size();
    }

    std::vector<std::vector<uint32_t>>& GetBatchTrainVertices_(){
        return batchTrainVertices_;
    }

    idx_t GetParts(){
        return target_parts_;
    }

    void clear(){
        batchTrainVertices_.clear();
    }

    void GetAllVertexPartionWithTrainBalance(){
        int test = -1;
        SetTrainVerticesWeight();
        if(this->GetParts() > 8){
            LOG_INFO("balance KwayPartition Method");
            test = METIS_PartGraphKway(&vertices_,&balance_constraint_, xadj_.data(), adjncy_.data(),
                        vweight_.data(), NULL, NULL, &target_parts_, NULL,
                        NULL, NULL, &objval_, part_.data());
        }else{
            LOG_INFO("balance RecursivePartition Method");
            test = METIS_PartGraphRecursive(&vertices_,&balance_constraint_, xadj_.data(), adjncy_.data(),
				       vweight_.data(), NULL, NULL, &target_parts_, NULL,
       				  NULL, NULL, &objval_, part_.data());
        }
        if(test == METIS_OK){
            std::cout<<"METIS_OK"<<std::endl;
        }else if(test == METIS_ERROR_INPUT){
            std::cout<<"METIS_ERROR_INPUT"<<std::endl;
        }else if(test == METIS_ERROR_MEMORY){
            std::cout<<"METIS_ERROR_MEMORY"<<std::endl;
        }else if(test == METIS_ERROR){
            std::cout<<"METIS_ERROR"<<std::endl;
        }
    }

    std::vector<idx_t>& GetPartitionResult(){
        // std::cout << "metis result size :" << part_.size() << std::endl << "metis result : ";
        // for (int i = 0; i < part_.size(); i++){
        //     std::cout << part_[i] << " ";
        // }
        // std::cout << std::endl;
        return part_;
    }

    // add by fuzb
    void Graph_Partition_With_Multi_Dim_Balance(int dim){
        int test = -1;
        set_multi_dimension_vertices_weight(dim);
        if(this->GetParts() > 8){
            LOG_INFO("balance KwayPartition Method");
            test = METIS_PartGraphKway(&vertices_,&balance_constraint_, xadj_.data(), adjncy_.data(),
                        vweight_.data(), NULL, NULL, &target_parts_, NULL,
                        NULL, NULL, &objval_, part_.data());
        }else{
            LOG_INFO("balance RecursivePartition Method");
            test = METIS_PartGraphRecursive(&vertices_,&balance_constraint_, xadj_.data(), adjncy_.data(),
				       vweight_.data(), NULL, NULL, &target_parts_, NULL,
       				  NULL, NULL, &objval_, part_.data());
        }
        if(test == METIS_OK){
            std::cout<<"METIS_OK"<<std::endl;
        }else if(test == METIS_ERROR_INPUT){
            std::cout<<"METIS_ERROR_INPUT"<<std::endl;
        }else if(test == METIS_ERROR_MEMORY){
            std::cout<<"METIS_ERROR_MEMORY"<<std::endl;
        }else if(test == METIS_ERROR){
            std::cout<<"METIS_ERROR"<<std::endl;
        }
    }

    void SetValVertices(std::vector<uint32_t> & valVerticesId){
        valVerticesId_ = valVerticesId;
    }

    void SetTestVertices(std::vector<uint32_t> & testVerticesId){
        testVerticesId_ = testVerticesId;
    }

    void set_degree()
    {
        degree_.resize(vertices_, 0);
        #pragma omp parallel for //注释掉多线程是为了测试输出
        for(int i = 0; i < vertices_; i++)
        {
            degree_[i] = xadj_[i+1] - xadj_[i];
            // std::cout << "id[" << i <<"]degree: " << degree_[i] << std::endl;
        }
    }

    void set_pagerange_score(double* pr)
    {
        pagerank_score_.resize(vertices_, 0);
        #pragma omp parallel for //注释掉多线程是为了测试输出
        for(int i = 0; i < vertices_; i++)
        {
            // std::cout << "pr[" << i << "]: " << pr[i] << std::endl;
            pagerank_score_[i] = round(pr[i] * 100);
            // std::cout << "pagerank_score: " << pagerank_score_[i] << std::endl;
        }
    }

    void set_multi_dimension_vertices_weight(int dim){
        if(dim == 10)// dgl的实现方式
        {
            dim = 4;
            balance_constraint_ = 4;
            vweight_.resize(vertices_ * dim, 0);
            
            #pragma omp parallel for //注释掉多线程是为了测试输出
            for(int i = 0; i < dim * vertices_; i = i + dim)
            {
                int vertexId = i/dim;
                if(!is_exist(trainVerticesId_, vertexId))
                {
                    vweight_[i] = 1;
                    vweight_[i + 2] = degree_[vertexId];
                }
                else
                {
                    vweight_[i + 1] = 1;
                    vweight_[i + 3] = degree_[vertexId];
                }
                // 输出权重，看看啥玩意
                // std::cout << "dgl: id[" << vertexId  << "]weight: ";
                // for(int j = 0; j < dim; j++){
                //     std::cout << vweight_[i+j] << " ";
                // }
                // std::cout << std::endl;
            }
        }
        else//nts的权重
        {
            int flag = dim;
            if(dim == 5)//第四维换成pagerank
            {
                dim = 4;
            }
            balance_constraint_ = dim;//set dim
            vweight_.resize(vertices_ * dim, 0);
            //the weight of train vertices
            #pragma omp parallel for //注释掉多线程是为了测试输出
            for(int i = 0; i < dim * vertices_; i = i + dim){
                int vertexId = i/dim;
                if(is_exist(trainVerticesId_, vertexId)){
                    vweight_[i] = 1;
                }
                else if(is_exist(valVerticesId_, vertexId)){
                    if(dim > 2)
                        vweight_[i + 1] = 1;
                }
                else if(is_exist(testVerticesId_, vertexId)){
                    if(dim > 2)
                        vweight_[i + 2] = 1;
                }
                if(dim == 2)
                    vweight_[i + 1] = degree_[vertexId];
                // else if(dim == 3)
                //     vweight_[i + 2] = degree_[vertexId];
                else if(flag == 4)
                    vweight_[i + 3] = degree_[vertexId];
                else if(flag == 5)
                    vweight_[i + 3] = pagerank_score_[vertexId];

                // std::cout << "id[" << vertexId  << "]weight: ";
                // for(int j = 0; j < dim; j++){
                //     std::cout << vweight_[i+j] << " ";
                // }
                // std::cout << std::endl;
            }
            // std::cout << "vweight_.size(): " << vweight_.size() << std::endl;
        }
    }


  std::vector<uint32_t> trainMaskVertex;
  std::vector<uint32_t> testMaskVertex;
  std::vector<uint32_t> valMaskVertex;
private:
  idx_t vertices_;
  idx_t balance_constraint_;
  idx_t target_parts_;
  idx_t objval_;
  std::vector<idx_t> xadj_;
  std::vector<idx_t> adjncy_;
  std::vector<idx_t> part_;
  std::vector<std::vector<uint32_t>> batchTrainVertices_;
  std::vector<uint32_t> trainVerticesId_;
  std::vector<idx_t> vweight_;

  idx_t testVertices;

  //add by fuzb
  std::vector<idx_t> degree_;
  std::vector<idx_t> pagerank_score_;
  std::vector<uint32_t> valVerticesId_;
  std::vector<uint32_t> testVerticesId_;
};

#endif
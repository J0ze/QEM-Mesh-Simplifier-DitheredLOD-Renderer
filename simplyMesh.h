#pragma once
#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <queue>
#include <unordered_set>
using namespace std;

#pragma region 基础数据结构
/// <summary>
/// 顶点数据
/// </summary>
struct Vertex {
    glm::vec3 Position;
    //glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int ID;
    string Type;
    string path;
};


/// <summary>
/// 边数据
/// </summary>
struct Edge {
    unsigned int v1, v2; // 点 要求v1严格小于v2
    float cost; // 折叠损耗
    Vertex newVertex; // 折叠后产生的新顶点

    // 脏数据处理 防止面拉丝问题
    unsigned int version_v1;
    unsigned int version_v2;

    // 重载运算符用于排序比较
    bool operator > (const Edge& other) const {
        return cost > other.cost;
    }
};

class SimplyMesh {
public:
    vector<Vertex> Vertices; // 网格的顶点数据集
    vector<unsigned int> Indices; // 网格的原索引集合
    vector<Texture> Textures;

    vector<glm::mat4> Vertices_Q;
    vector<unordered_set<unsigned int>> neighbors; // 顶点邻接表
    
    vector<bool> isValid; // 顶点存活数组
    vector<unsigned int> vertexVersion; // 版本追踪

    // 新建网格数据集合
    vector<Vertex> newVertices;
    vector<unsigned int> newIndices;

    // 构造函数
    SimplyMesh(vector<Vertex> &vertices, vector<unsigned int> &indices, vector<Texture> &textures, float LODratio);

    void Draw(Shader& shader);

private:
    unsigned VAO, VBO, EBO;

    void InitailizeQuadrics();
    void InitailizeNeighbors();
    void Simply(float LODratio);
    void SetupMesh();
};
#pragma endregion

#pragma region 前向函数声明
Edge CaculateCost(unsigned int v1, unsigned int v2, const vector<Vertex>& vertices, const vector<glm::mat4> &Quadric, const vector<unsigned int>& vertexVersion);
unsigned int getFinalVertex(vector<unsigned int>& aliasTable, unsigned int vertex);
#pragma endregion

#pragma region 核心成员函数实现
SimplyMesh::SimplyMesh(vector<Vertex> &vertices, vector<unsigned int> &indices, vector<Texture> &textures, float LODratio) {
    this->Vertices = vertices;
    this->Indices = indices;
    this->Textures = textures;

    Vertices_Q.assign(Vertices.size(), glm::mat4(0.0f));
    neighbors.resize(Vertices.size());
    isValid.assign(Vertices.size(), true);
    vertexVersion.assign(Vertices.size(), 0);

    InitailizeQuadrics();   
    InitailizeNeighbors();
    Simply(LODratio);
    SetupMesh();
}

void SimplyMesh::SetupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, newVertices.size() * sizeof(Vertex), &newVertices[0], GL_STATIC_DRAW);

    // 绑定的是LOD缓冲
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, newIndices.size() * sizeof(unsigned int), &newIndices[0], GL_STATIC_DRAW);

    // 绑定顶点内存结构
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // 这里的法线实际上已经变化了 但是因为暂时用不到光照处理 所以我们搁置
    //glEnableVertexAttribArray(1);
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
}

void SimplyMesh::Draw(Shader& shader) {
    unsigned int diffuseNr = 1; // 漫反射材质编号
    
    // 遍历材质数组中的所有材质
    for (unsigned int i = 0; i < Textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i); // 启用对应存储在GPU上的纹理单元插槽
        string number;
        string name = Textures[i].Type;
        if (name == "texture_diffuse") {
            number = to_string(diffuseNr++);
        }

        shader.setInt((name + number).c_str(), i); // 传入对应的材质
        glBindTexture(GL_TEXTURE_2D, Textures[i].ID);
    }

    glActiveTexture(GL_TEXTURE0);

    // 绘制网格
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, newIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// 迭代ver::2.01
// 初始化二次型矩阵
// 遍历所有三角面 取得每个点目前的二次型影响度
void SimplyMesh::InitailizeQuadrics() {
    for (unsigned int i = 0; i < Indices.size(); i += 3) {
        unsigned int v0 = Indices[i];
        unsigned int v1 = Indices[i + 1];
        unsigned int v2 = Indices[i + 2];

        glm::vec3 p0 = Vertices[v0].Position;
        glm::vec3 p1 = Vertices[v1].Position;
        glm::vec3 p2 = Vertices[v2].Position;

        // 计算三角面的法线(a, b, c)
        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

        // 得到ax + by + cz + d = 0 平面表达公式中的距离d
        float d = -glm::dot(normal, p0);

        // 构建四维平面法向量 引入距离d
        glm::vec4 p(normal.x, normal.y, normal.z, d);

        // 计算外积作为二次型矩阵  p 乘上 p的转置 也就是一个4 X 4矩阵
        glm::mat4 Quad = glm::outerProduct(p, p);

        // 将二次型矩阵累加到对应顶点的Quadrics上
        Vertices_Q[v0] += Quad;
        Vertices_Q[v1] += Quad;
        Vertices_Q[v2] += Quad;
    }
}

// 初始化邻接表 得到每个顶点相连接的点
void SimplyMesh::InitailizeNeighbors() {
    for (int i = 0; i < Indices.size(); i += 3) {
        unsigned int v1 = Indices[i];
        unsigned int v2 = Indices[i + 1];
        unsigned int v3 = Indices[i + 2];

        neighbors[v1].insert(v2); neighbors[v1].insert(v3);
        neighbors[v2].insert(v1); neighbors[v2].insert(v3);
        neighbors[v3].insert(v1); neighbors[v3].insert(v2);
    }
}

void SimplyMesh::Simply(float LODratio) {
    unsigned int targetTrianglesNums = static_cast<unsigned int>(Indices.size() * LODratio);
    targetTrianglesNums -= targetTrianglesNums % 3;

    // 制作并查集 存储简化折叠后的顶点最终映射
    vector<unsigned int> aliasTable(Vertices.size());
    for (int i = 0; i < Vertices.size(); i++) {
        aliasTable[i] = i;
    }

    priority_queue<Edge, vector<Edge>, greater<Edge>> minHeap;
    for (int i = 0; i < Indices.size(); i += 3) {
        unsigned int v1 = Indices[i];
        unsigned int v2 = Indices[i + 1];
        unsigned int v3 = Indices[i + 2];

        minHeap.push(CaculateCost(v1, v2, Vertices, Vertices_Q, vertexVersion));
        minHeap.push(CaculateCost(v2, v3, Vertices, Vertices_Q, vertexVersion));
        minHeap.push(CaculateCost(v1, v3, Vertices, Vertices_Q, vertexVersion));
    }

    unsigned int orignalTriganleNums = Indices.size() / 3;
    int collapsesNeeded = (orignalTriganleNums - targetTrianglesNums / 3) / 2;
    int collapsesDone = 0;

    while (!minHeap.empty() && collapsesDone < collapsesNeeded) {
        Edge edge = minHeap.top();
        minHeap.pop();

        unsigned int v1 = edge.v1;
        unsigned int v2 = edge.v2;

        // 如果边中有任意一个点已经处理了 就直接跳过
        if (!isValid[v1] || !isValid[v2]) {
            continue;
        }
        
        // 脏数据剔除
        if (edge.version_v1 != vertexVersion[v1] || edge.version_v2 != vertexVersion[v2]) {
            continue; // 版本不匹配，说明这是历史遗留的脏边，直接扔掉
        }

        // 如果v1 和 v2已经不再相连 则直接跳过（已经通过惰性剔除的方法以新边的形式压入了）
        if (neighbors[v1].find(v2) == neighbors[v1].end()) {
            continue;
        }

        // 更新点版本
        vertexVersion[v1]++;
        vertexVersion[v2]++;

        // 更新顶点 让任意一个更新过来就行
        Vertices[v1] = edge.newVertex;

        Vertices_Q[v1] = Vertices_Q[v1] + Vertices_Q[v2]; // 继承误差 加倍删除面权重 防止过分偏离
        // 记录v2被删除点的消失
        isValid[v2] = false;

        // 更新并查集
        aliasTable[v2] = v1;

        // 拓扑重建
        for (unsigned int v : neighbors[v2]) {
            if (v == v1) continue;
            neighbors[v].erase(v2);
            neighbors[v].insert(v1);
            neighbors[v1].insert(v);
        }

        neighbors[v2].clear();

        // 重新评估误差 并压入融并后的新边 进行惰性剔除
        for (unsigned int n : neighbors[v1]) {
            Edge newEdge = CaculateCost(v1, n, Vertices, Vertices_Q, vertexVersion);
            minHeap.push(newEdge);
        }

        collapsesDone++;
    }

    // 重构数据集
    vector<unsigned int> liveVertexIndex(Vertices.size(), -1);
    unsigned int currentNewIndex = 0;

    for (unsigned int i = 0; i < Vertices.size(); i++) {
        if (isValid[i]) {
            newVertices.push_back(Vertices[i]);
            liveVertexIndex[i] = currentNewIndex++; // 记录老的存活顶点对应的新表索引
        }
    }

    for (unsigned int i = 0; i < Indices.size(); i += 3) {
        unsigned int v0 = Indices[i];
        unsigned int v1 = Indices[i + 1];
        unsigned int v2 = Indices[i + 2];

        // 找到旧的面各点对应的新索引
        // 如果退化为线或者点则直接跳过
        unsigned int v0_new = getFinalVertex(aliasTable, v0);
        unsigned int v1_new = getFinalVertex(aliasTable, v1);
        unsigned int v2_new = getFinalVertex(aliasTable, v2);

        if (v0_new == v1_new || v0_new == v2_new || v1_new == v2_new) {
            continue;
        }

        // 将存活面的索引映射到新的索引表
        newIndices.push_back(liveVertexIndex[v0_new]);
        newIndices.push_back(liveVertexIndex[v1_new]);
        newIndices.push_back(liveVertexIndex[v2_new]);
    }

}
#pragma endregion

#pragma region 辅助函数
// 计算单个边折叠需要的代价
Edge CaculateCost(unsigned int v1, unsigned int v2, const vector<Vertex>& vertices, const vector<glm::mat4> &Quadric, const vector<unsigned int>& vertexVersion) {
    Edge edge;
    if (v1 > v2) std::swap(v1, v2);
    edge.v1 = v1;
    edge.v2 = v2;

    // 打上版本戳
    edge.version_v1 = vertexVersion[v1];
    edge.version_v2 = vertexVersion[v2];

    Vertex vt1 = vertices[v1];
    Vertex vt2 = vertices[v2];

    glm::mat4 Q_merge = Quadric[v1] + Quadric[v2];

    // 从合并到 v1 合并到v2 合并到二者中点 三选项中选出cost最小的一个
    Vertex opt1 = vt1;
    Vertex opt2 = vt2;
    Vertex opt3;
    opt3.Position = (vt1.Position + vt2.Position) * 0.5f;
    opt3.TexCoords = (vt1.TexCoords + vt2.TexCoords) * 0.5f;

    auto evaluateError = [](const glm::mat4 Q, const glm::vec3& v) {
        // 将顶点转化为四维表示
        glm::vec4 v4(v, 1.0f);
        return glm::dot(v4, Q * v4);
    };

    float cost1 = evaluateError(Q_merge, opt1.Position);
    float cost2 = evaluateError(Q_merge, opt2.Position);
    float cost3 = evaluateError(Q_merge, opt3.Position);

    // 找出边合并的最小代价
    edge.cost = cost1;
    edge.newVertex = opt1;
    if (edge.cost > cost2) { edge.cost = cost2; edge.newVertex = opt2; }
    if (edge.cost > cost3) { edge.cost = cost3; edge.newVertex = opt3; }

    return edge;
}

// 并查集函数 查找最终映射顶点
unsigned int getFinalVertex(vector<unsigned int>& aliasTable, unsigned int vertex) {
    if (vertex == aliasTable[vertex]) return vertex;
    return aliasTable[vertex] = getFinalVertex(aliasTable, aliasTable[vertex]);
}
#pragma endregion


